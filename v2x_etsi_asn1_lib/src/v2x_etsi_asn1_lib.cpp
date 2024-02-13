#include "v2x_etsi_asn1_lib/v2x_etsi_asn1_lib.h"
#include <v2x_amqp_connector_lib/v2x_amqp_connector_lib.h>
#include "v2x_etsi_asn1_lib/time_conversions.h"
#include <chrono>
#include <vector>

namespace mrm::v2x_etsi_asn1_lib
{

const std::map<ETSIMessageType, std::string> ETSIAMQPTransceiverBase::type2str = {
  { ETSIMessageType::DENM, "denm" }, { ETSIMessageType::CPM, "cpm" }, { ETSIMessageType::CAM, "cam" },
  { ETSIMessageType::VAM, "vam" },   { ETSIMessageType::MCM, "mcm" },
};

ETSIAMQPTransceiverBase::ETSIAMQPTransceiverBase()
{
  etsi_msg_handlers_ = {
    { ETSIMessageType::CAM,
      { &asn_DEF_CAM,
        [this](auto& msg, const BinaryETSIMessage& msg_bin) {
          handleCAM(std::static_pointer_cast<CAM>(msg), msg_bin);
        } } },
    { ETSIMessageType::VAM,
      { &asn_DEF_VAM,
        [this](auto& msg, const BinaryETSIMessage& msg_bin) {
          handleVAM(std::static_pointer_cast<VAM>(msg), msg_bin);
        } } },
    { ETSIMessageType::CPM,
      { &asn_DEF_CollectivePerceptionMessage,
        [this](auto& msg, const BinaryETSIMessage& msg_bin) {
          handleCPM(std::static_pointer_cast<CollectivePerceptionMessage>(msg), msg_bin);
        } } },
    { ETSIMessageType::MCM,
      { &asn_DEF_MCM,
        [this](auto& msg, const BinaryETSIMessage& msg_bin) {
          handleMCM(std::static_pointer_cast<MCM>(msg), msg_bin);
        } } },
  };
}

ETSIAMQPTransceiverBase::~ETSIAMQPTransceiverBase()
{
  disconnect();
}

void ETSIAMQPTransceiverBase::connect(StationId_t station_id,
                                      std::string url,
                                      std::string address_rx,
                                      std::string address_tx,
                                      std::string user,
                                      std::string pw,
                                      std::string filter_query)
{
  assert(client_ == nullptr);
  station_id_ = station_id;
  client_ =
      std::make_shared<mrm::v2x_amqp_connector_lib::AMQPClient>(url, address_rx, address_tx, user, pw, filter_query);
  receiver_thread_ = std::make_shared<std::thread>([&]() -> void {
    while (true)
    {
      auto msg = client_->receive();
      if (!msg)
      {
        if (client_->is_closing())
        {
          break;
        }
        // wait for reconnection
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);
        continue;
      }
      handleMessage(*msg);
    }
  });
}

void ETSIAMQPTransceiverBase::disconnect()
{
  if (client_)
  {
    client_.reset();
    receiver_thread_->join();
    receiver_thread_.reset();
  }
}

void ETSIAMQPTransceiverBase::handleMessage(const proton::message& message)
{
  LOG_DEB("Num properties: " << message.properties().size());
  LOG_DEB("TTL: " << message.ttl().milliseconds() << " ms");

  BinaryETSIMessage bin_msg;

  if (!message.properties().exists("mid"))
  {
    LOG_WARN_THROTTLE(5.0, "Received a message without 'mid' field!");
    return;
  }
  auto mid = proton::get<uint16_t>(message.properties().get("mid"));
  bin_msg.message_type = static_cast<ETSIMessageType>(mid);
  auto msg_type_str = type2str.find(bin_msg.message_type);
  if (msg_type_str == type2str.end())
  {
    LOG_WARN_THROTTLE(5.0, "Message type unknown: " << mid);
    return;
  }
  if (message.subject() != msg_type_str->second)
  {
    LOG_WARN_THROTTLE(5.0,
                      "Message subject does not match mid: " << message.subject() << " vs. " << msg_type_str->second
                                                             << " (from mid)");
    return;
  }

  if (!message.properties().exists("station_id"))
  {
    LOG_WARN_THROTTLE(5.0, "Received a message without 'station_id' field!");
    return;
  }
  auto station_id = proton::get<uint32_t>(message.properties().get("station_id"));
  LOG_DEB("Station ID was: " << station_id);
  bin_msg.station_id = station_id;

  if (message.properties().exists("destination_station_id"))
  {
    auto destination = proton::get<uint32_t>(message.properties().get("destination_station_id"));
    LOG_DEB("Destination station ID was: " << destination);
    bin_msg.destination_station_id = destination;
  }

  auto ct = message.creation_time();
  LOG_DEB("creation_time was: " << ct);
  bin_msg.time = std::chrono::system_clock::time_point{ std::chrono::milliseconds{ ct.milliseconds() } };

  auto body_type = message.body().type();
  LOG_DEB("Got message with type " << static_cast<int>(mid) << ", encoding type " << body_type);

  std::string content;
  if (body_type == proton::BINARY)
  {
    proton::binary content_bin;
    proton::get(message.body(), content_bin);
    content = content_bin;
  }
  else if (body_type == proton::STRING)
  {
    proton::get(message.body(), content);
  }
  else
  {
    LOG_ERR_THROTTLE(5.0, "Got unknown body encoding " << body_type << "! Skipping message!");
    return;
  }

  bin_msg.data.reserve(content.size());
  std::copy(content.begin(), content.end(), std::back_inserter(bin_msg.data));

  handleBinaryMessage(bin_msg);
}

void ETSIAMQPTransceiverBase::handleBinaryMessage(const BinaryETSIMessage& msg)
{
  LOG_DEB("Got binary ETSI message: " << msg.message_type);
  auto handler = etsi_msg_handlers_.find(msg.message_type);
  if (handler == etsi_msg_handlers_.end())
  {
    LOG_WARN("No handler registered for this ETSI message type: " << static_cast<int>(msg.message_type));
    return;
  }

  const auto* type = std::get<0>(handler->second);
  auto handler_f = std::get<1>(handler->second);
  void* pMsg = nullptr;

  auto ret = uper_decode_complete(nullptr, type, &pMsg, msg.data.data(), msg.data.size());
  auto msg_ptr = std::shared_ptr<void>(pMsg, [type](void* data) { ASN_STRUCT_FREE(*type, data); });

  if (ret.code != RC_OK)
  {
    LOG_ERR_THROTTLE(5.0, "Decoding of " << msg.message_type << " failed!");
    return;
  }

  handler_f(msg_ptr, msg);
}

bool ETSIAMQPTransceiverBase::sendETSIMsg(const asn_TYPE_descriptor_t* type,
                                          const ETSIMessageType message_type,
                                          void* pMsg,
                                          std::optional<StationId_t> destination_station_id)
{
  if (!is_sender_connected())
  {
    return false;
  }
  LOG_DEB("sendETSIMsg: " << message_type);
  auto res = asn_encode_to_new_buffer(nullptr, ATS_UNALIGNED_BASIC_PER, type, pMsg);
  if (res.buffer == nullptr || res.result.encoded < 0)
  {
    LOG_WARN_THROTTLE(5.0, "UPER encoding of message failed!");
    return false;
  }

  bool ret = sendEncodedETSIMsg(
      static_cast<const char*>(res.buffer), res.result.encoded, message_type, destination_station_id);
  free(res.buffer);
  return ret;
}

bool ETSIAMQPTransceiverBase::sendEncodedETSIMsg(const char* buffer,
                                                 size_t size,
                                                 const ETSIMessageType message_type,
                                                 std::optional<StationId_t> destination_station_id)
{
  proton::message message;
  message.body() = proton::binary(std::string(const_cast<char*>(buffer), size));
  message.ttl(message_type != ETSIMessageType::CPM ? proton::duration::SECOND : proton::duration(100));  // 100 ms
  message.subject(type2str.at(message_type));

  proton::timestamp timestamp;
  const auto now =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  timestamp = now / 1'000'000;
  message.creation_time(timestamp);

  message.properties().put("mid", (uint16_t)message_type);
  message.properties().put("station_id", static_cast<uint32_t>(station_id_));
  if (destination_station_id)
  {
    message.properties().put("destination_station_id", static_cast<uint32_t>(*destination_station_id));
  }

  if (!is_sender_connected() || !client_->send(message))
  {
    LOG_ERR_THROTTLE(5.0, "Error sending " << type2str.at(message_type) << " message...");
    return false;
  }
  LOG_DEB("Sending " << message_type << " message with station_id " << station_id_);
  return true;
}

uint64_t ETSIAMQPTransceiverBase::decodeTimestampIts(const TimestampIts_t* timestamp)
{
  uint64_t ret;
  asn_INTEGER2umax(timestamp, &ret);
  return ret;
}

void ETSIAMQPTransceiverBase::encodeTimestampIts(uint64_t timestamp, TimestampIts_t* timestamp_out)
{
  asn_umax2INTEGER(timestamp_out, timestamp);
}

void ETSIAMQPTransceiverBase::handleCPM(const std::shared_ptr<const CollectivePerceptionMessage>& msg,
                                        const BinaryETSIMessage& msg_bin)
{
  int total_msg_segments = 1;
  int segment_num = 1;
  if (msg->payload.managementContainer.segmentationInfo != nullptr)
  {
    total_msg_segments = msg->payload.managementContainer.segmentationInfo->totalMsgNo;
    segment_num = msg->payload.managementContainer.segmentationInfo->thisMsgNo;
  }
  LOG_DEB("segment number: " << segment_num << " / " << total_msg_segments);

  const auto now =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  const uint64_t reconstructed_time_unix =
      ETSITime2UnixTime(decodeTimestampIts(&msg->payload.managementContainer.referenceTime));
  LOG_DEB("station ID: " << msg->header.stationId);

  if (received_cpm_msgs_.find(msg->header.stationId) == received_cpm_msgs_.end())
  {
    received_cpm_msgs_[msg->header.stationId] = {};
  }
  auto& station_msgs = received_cpm_msgs_[msg->header.stationId];
  if (station_msgs.find(reconstructed_time_unix) == station_msgs.end())
  {
    station_msgs[reconstructed_time_unix] = {};
  }
  auto& cpm_msgs = station_msgs[reconstructed_time_unix];
  cpm_msgs[segment_num] = msg;

  if (cpm_msgs.size() == total_msg_segments)
  {
    LOG_INF("Received full CPM");
    handleCompleteCPM(msg->header.stationId, cpm_msgs, reconstructed_time_unix);
    station_msgs.erase(reconstructed_time_unix);
  }

  // discard old CPMs
  static const auto max_keep_time = 60'000'000'000ULL;
  for (auto& [station_id, station_id_cpms] : received_cpm_msgs_)
  {
    for (auto& [r_time, cpm_segments] : station_id_cpms)
    {
      if (r_time < now - max_keep_time)
      {
        assert(!cpm_segments.empty());
        const auto& seg_info = cpm_segments.begin()->second->payload.managementContainer.segmentationInfo;
        const auto total_segments = seg_info->totalMsgNo;
        if (total_segments != cpm_segments.size())
        {
          LOG_WARN("Old incomplete CPM of station id " << station_id << " detected with time " << r_time << " (has "
                                                       << cpm_segments.size() << " segments, but " << total_segments
                                                       << " were announced (this is segment " << seg_info->thisMsgNo
                                                       << ")");
        }
      }
    }
    auto erased = std::erase_if(station_id_cpms, [&](const auto& x) { return x.first < now - max_keep_time; });
    LOG_DEB("erased " << erased << " old cpm msgs from station_id " << std::to_string(station_id));
  }
}

void ETSIAMQPTransceiverBase::handleCAM(const std::shared_ptr<const CAM>& msg, const BinaryETSIMessage& msg_bin)
{
}
void ETSIAMQPTransceiverBase::handleVAM(const std::shared_ptr<const VAM>& msg, const BinaryETSIMessage& msg_bin)
{
}
void ETSIAMQPTransceiverBase::handleCompleteCPM(
    StationId_t station_id,
    std::map<uint8_t, std::shared_ptr<const CollectivePerceptionMessage>> msgs,
    uint64_t time)
{
}
void ETSIAMQPTransceiverBase::handleMCM(const std::shared_ptr<const MCM>& msg, const BinaryETSIMessage& msg_bin)
{
}

bool ETSIAMQPTransceiverBase::is_sender_connected()
{
  return client_->is_sender_connected();
}

}  // namespace mrm::v2x_etsi_asn1_lib
