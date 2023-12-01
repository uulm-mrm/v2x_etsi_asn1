#ifndef LIBRARY_INCLUDE_V2X_ETSI_ASN1_LIB_HPP_
#define LIBRARY_INCLUDE_V2X_ETSI_ASN1_LIB_HPP_

#include <aduulm_logger/aduulm_logger.hpp>
#include <v2x_amqp_connector_lib/v2x_amqp_connector_lib.h>
#include <v2x_etsi_asn1_lib/time_conversions.h>
#include <CollectivePerceptionMessage.h>
#include <CAM.h>
#include <MCM.h>
#include <VAM.h>

namespace mrm::v2x_etsi_asn1_lib
{
namespace enums
{
enum ETSIMessageType_t
{
  DENM = 2048,
  CPM = 2049,
  CAM = 2050,
  VAM = 2051,
  MCM = 2052,
};
}
using ETSIMessageType = enums::ETSIMessageType_t;

struct BinaryETSIMessage
{
  ETSIMessageType message_type{};
  StationId_t station_id{};
  std::optional<StationId_t> destination_station_id{};
  std::chrono::system_clock::time_point time{};
  std::vector<uint8_t> data{};
};

template <class T>
static inline std::shared_ptr<T> allocateETSIMsg(const asn_TYPE_descriptor_t& type)
{
  return { static_cast<T*>(calloc(1, sizeof(T))), [type](T* msg) { ASN_STRUCT_FREE(type, msg); } };
}

class ETSIAMQPTransceiverBase
{
public:
  ETSIAMQPTransceiverBase();
  virtual ~ETSIAMQPTransceiverBase();

  void connect(StationId_t station_id,
               std::string url,
               std::string address_rx,
               std::string address_tx,
               std::string user,
               std::string pw,
               std::string filter_query = "");
  void disconnect();
  bool sendETSIMsg(const asn_TYPE_descriptor_t* type,
                   ETSIMessageType message_type,
                   void* pMsg,
                   std::optional<StationId_t> destination_station_id = {});
  bool sendEncodedETSIMsg(const char* buffer,
                          size_t size,
                          ETSIMessageType message_type,
                          std::optional<StationId_t> destination_station_id = {});

  static uint64_t decodeTimestampIts(const TimestampIts_t* timestamp);
  static void encodeTimestampIts(uint64_t timestamp, TimestampIts_t* timestamp_out);

  static const std::map<ETSIMessageType, std::string> type2str;
  bool is_sender_connected();

protected:
  virtual void handleCAM(const std::shared_ptr<const CAM>& msg, const BinaryETSIMessage& msg_bin);
  virtual void handleVAM(const std::shared_ptr<const VAM>& msg, const BinaryETSIMessage& msg_bin);
  virtual void handleCPM(const std::shared_ptr<const CollectivePerceptionMessage>& msg,
                         const BinaryETSIMessage& msg_bin);
  virtual void handleCompleteCPM(StationId_t station_id,
                                 std::map<uint8_t, std::shared_ptr<const CollectivePerceptionMessage>> msgs,
                                 uint64_t time);
  virtual void handleMCM(const std::shared_ptr<const MCM>& msg, const BinaryETSIMessage& msg_bin);

  virtual void handleMessage(const proton::message& message);
  virtual void handleBinaryMessage(const BinaryETSIMessage& message);

  StationId_t station_id_;
  std::map<ETSIMessageType,
           std::tuple<const asn_TYPE_descriptor_t*,
                      const std::function<void(std::shared_ptr<void>&, const BinaryETSIMessage&)>>>
      etsi_msg_handlers_;

private:
  std::shared_ptr<mrm::v2x_amqp_connector_lib::AMQPClient> client_;
  std::shared_ptr<std::thread> receiver_thread_;

  std::map<StationId_t, std::map<uint64_t, std::map<uint8_t, std::shared_ptr<const CollectivePerceptionMessage>>>>
      received_cpm_msgs_;
};
}  // namespace mrm::v2x_etsi_asn1_lib

#endif /* LIBRARY_INCLUDE_V2X_ETSI_ASN1_LIB_HPP_ */
