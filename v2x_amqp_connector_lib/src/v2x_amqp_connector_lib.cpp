#include <v2x_amqp_connector_lib/v2x_amqp_connector_lib.h>
#include <proton/receiver_options.hpp>
#include <proton/source_options.hpp>
#include <proton/connection_options.hpp>
#include <proton/reconnect_options.hpp>
#include <proton/codec/encoder.hpp>
#include <proton/source.hpp>
#include <thread>

namespace mrm::v2x_amqp_connector_lib
{
AMQPClient::AMQPClient(std::string url,
                       std::string address_rx,
                       std::string address_tx,
                       std::string user,
                       std::string pw,
                       std::string filter_query)
  : url_(std::move(url))
  , address_rx_(std::move(address_rx))
  , address_tx_(std::move(address_tx))
  , user_(std::move(user))
  , pw_(std::move(pw))
  , filter_query_(std::move(filter_query))
{
  LOG_DEB("AMQPClient initialized");
  container_thread_ = std::make_shared<std::thread>([&]() {
    while (!closing_)
    {
      LOG_INF("Starting new container");
      container_ = std::make_shared<proton::container>(*this);
      container_->run();
      LOG_INF("Container stopped.");
      closing_connection_ = false;
      sender_connected_ = false;
      sender_.reset();
      connection_.reset();
      work_queue_ = nullptr;
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(200ms);
    }
  });
}

AMQPClient::~AMQPClient()
{
  closing_ = true;
  close_connection();
  container_->stop();
  container_thread_->join();
  container_.reset();
  container_thread_.reset();
}

bool AMQPClient::send(const proton::message& msg)
{
  if (!sender_connected_)
  {
    LOG_WARN_THROTTLE(5., "sender not connected");
    return false;
  }
  // Use [=] to copy the message, we cannot pass it by reference since it
  // will be used in another thread.
  work_queue()->add([=]() {
    if (!sender_)
    {
      LOG_DEB("sender not yet created");
      return;
    }
    LOG_DEB("sending message");
    sender_->send(msg);
  });
  return true;
}

std::optional<proton::message> AMQPClient::receive()
{
  std::unique_lock<std::mutex> l(lock_);
  while (messages_.empty() && !closing_connection_ && !closing_)
  {
    messages_ready_.wait(l);
  }
  if (closing_connection_ || closing_)
  {
    return {};
  }
  auto msg = std::move(messages_.front());
  messages_.pop();
  return { msg };
}

void AMQPClient::close_connection()
{
  LOG_INF("Closing connection");
  {
    std::lock_guard<std::mutex> l(lock_);
    closing_connection_ = true;
    messages_ready_.notify_all();
  }
  if (sender_connected_)
  {
    work_queue()->add([=]() { sender_->connection().close(); });
  }
  container_->stop();
}

proton::work_queue* AMQPClient::work_queue()
{
  std::unique_lock<std::mutex> l(lock_);
  while (work_queue_ == nullptr)
  {
    sender_ready_.wait(l);
  }
  return work_queue_;
}

void AMQPClient::on_container_start(proton::container& cont)
{
  proton::reconnect_options ro;
  proton::connection_options co;
  if (user_ == "anonymous")
  {
    co.sasl_enabled(true);
    co.sasl_allowed_mechs("ANONYMOUS");
    co.sasl_allow_insecure_mechs(true);
  }
  else if (!user_.empty())
  {
    co.sasl_enabled(true);
    co.sasl_allow_insecure_mechs(true);
    co.user(user_);
    co.password(pw_);
  }
  else
  {
    co.sasl_enabled(false);
  }
  ro.max_delay(proton::duration(2000));
  co.idle_timeout(proton::duration(5000));
  co.reconnect(ro);
  connection_ = cont.connect(url_, co);
  LOG_DEB("on_container_start done");
}

void AMQPClient::on_connection_open(proton::connection& conn)
{
  LOG_DEB("on_connection_open start");
  if (!address_tx_.empty())
  {
    open_senders();
  }
  if (!address_rx_.empty())
  {
    auto options = proton::source_options();
    if (!filter_query_.empty())
    {
      proton::source::filter_map filters;
      proton::symbol key("selector");
      proton::value res;
      proton::codec::encoder encoder{ res };
      encoder << proton::codec::start(proton::DESCRIBED, proton::NULL_TYPE, true)
              << proton::symbol("apache.org:selector-filter:string") << proton::value(filter_query_)
              << proton::codec::finish();
      filters.put(key, res);
      options = options.filters(filters);
    }
    auto opts = proton::receiver_options().source(options);
    conn.open_receiver(address_rx_, opts);
  }
  LOG_DEB("on_connection_open done");
}
void AMQPClient::on_sender_open(proton::sender& s)
{
  LOG_DEB("on_sender_open");
  // sender_ and work_queue_ must be set atomically
  std::lock_guard<std::mutex> l(lock_);
  sender_ = s;
  work_queue_ = &s.work_queue();
  sender_connected_ = true;
  LOG_DEB("on_sender_open done");
  sender_ready_.notify_all();
}
void AMQPClient::on_message(proton::delivery& dlv, proton::message& msg)
{
  std::lock_guard<std::mutex> l(lock_);
  messages_.push(msg);
  messages_ready_.notify_all();
}

void AMQPClient::open_senders()
{
  // Do initial per-connection setup here.
  // Open initial senders/receivers if needed.
  if (!connection_)
  {
    LOG_ERR("No connection, but trying to create sender!");
    return;
  }
  LOG_INF("Opening sender...");
  connection_->open_sender(address_tx_);
}

void AMQPClient::on_error(const proton::error_condition& e)
{
  LOG_ERR("unexpected error: " << e);
  close_connection();
}
void AMQPClient::on_transport_close(proton::transport& tp)
{
  LOG_WARN("transport closed");
  close_connection();
}
void AMQPClient::on_transport_error(proton::transport& tp)
{
  LOG_WARN("transport error");
  close_connection();
}
void AMQPClient::on_connection_close(proton::connection& conn)
{
  LOG_WARN("connection closed");
  close_connection();
}
void AMQPClient::on_connection_error(proton::connection& conn)
{
  LOG_WARN("connection closed due to error: " << conn.error());
  close_connection();
}
void AMQPClient::on_sender_close(proton::sender& s)
{
  LOG_WARN("sender closed");
  sender_connected_ = false;
}
void AMQPClient::on_sender_error(proton::sender& s)
{
  LOG_WARN("sender closed with error");
  close_connection();
}
void AMQPClient::on_sender_detach(proton::sender& s)
{
  LOG_WARN("sender detached/closed");
  close_connection();
}
[[nodiscard]] bool AMQPClient::is_sender_connected() const
{
  return sender_connected_;
}
[[nodiscard]] bool AMQPClient::is_closing() const
{
  return closing_;
}
}  // namespace mrm::v2x_amqp_connector_lib
