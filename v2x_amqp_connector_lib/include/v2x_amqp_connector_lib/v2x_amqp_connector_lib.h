#ifndef LIBRARY_INCLUDE_V2X_AMQP_CONNECTOR_LIB_HPP_
#define LIBRARY_INCLUDE_V2X_AMQP_CONNECTOR_LIB_HPP_

#include <aduulm_logger/aduulm_logger.hpp>

#include <proton/connection.hpp>
#include <proton/container.hpp>
#include <proton/message.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/receiver.hpp>
#include <proton/sender.hpp>
#include <proton/work_queue.hpp>

#include <condition_variable>
#include <queue>
#include <optional>

namespace mrm::v2x_amqp_connector_lib
{

class AMQPClient : public proton::messaging_handler
{
public:
  AMQPClient(std::string url,
             std::string address_rx,
             std::string address_tx,
             std::string user,
             std::string pw,
             std::string filter_query = "");
  ~AMQPClient() override;
  bool send(const proton::message& msg);
  std::optional<proton::message> receive();
  [[nodiscard]] bool is_sender_connected() const;
  [[nodiscard]] bool is_closing() const;

protected:
  void close_connection();

private:
  const std::string url_;
  const std::string address_rx_;
  const std::string address_tx_;
  const std::string user_;
  const std::string pw_;
  const std::string filter_query_;

  std::mutex lock_;
  std::optional<proton::connection> connection_;
  std::optional<proton::sender> sender_;
  proton::work_queue* work_queue_{};
  std::condition_variable sender_ready_;
  std::queue<proton::message> messages_;
  std::condition_variable messages_ready_;
  bool closing_connection_ = false;
  bool closing_ = false;
  bool sender_connected_ = false;

  std::shared_ptr<proton::container> container_;
  std::shared_ptr<std::thread> container_thread_;

  proton::work_queue* work_queue();
  void on_container_start(proton::container& cont) override;
  void on_connection_open(proton::connection& conn) override;
  void on_sender_open(proton::sender& s) override;
  void on_message(proton::delivery& dlv, proton::message& msg) override;

  void open_senders();
  void on_error(const proton::error_condition& e) override;
  void on_transport_close(proton::transport& tp) override;
  void on_transport_error(proton::transport& tp) override;
  void on_connection_close(proton::connection& conn) override;
  void on_connection_error(proton::connection& conn) override;
  void on_sender_close(proton::sender& s) override;
  void on_sender_error(proton::sender& s) override;
  void on_sender_detach(proton::sender& s) override;
};

}  // namespace mrm::v2x_amqp_connector_lib

#endif /* LIBRARY_INCLUDE_V2X_AMQP_CONNECTOR_LIB_HPP_ */
