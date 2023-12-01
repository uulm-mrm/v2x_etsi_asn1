#include <v2x_etsi_asn1_lib/v2x_etsi_asn1_lib.h>
#include <v2x_etsi_asn1_lib/utils.h>
#include <v2x_etsi_asn1_lib/units.h>
#include <v2x_etsi_asn1_lib/logger_setup.h>
#include <v2x_amqp_connector_lib/logger_setup.h>

namespace et = mrm::v2x_etsi_asn1_lib;

class ETSIAMQPTransceiver : public et::ETSIAMQPTransceiverBase
{
public:
  ETSIAMQPTransceiver()
  {
  }
  virtual ~ETSIAMQPTransceiver()
  {
  }

  // Fills a CAM message with dummy data and sends it
  void sendDummyCAM()
  {
    auto pMsg = et::allocateETSIMsg<CAM>(asn_DEF_CAM);

    pMsg->header.protocolVersion = 2;
    pMsg->header.messageId = MessageId_cam;
    pMsg->header.stationId = 101;

    // to have an accurate position information, the time should correspond to the measurement time
    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG_INF("Our time: " << now);
    pMsg->cam.generationDeltaTime = et::UnixTime2GenerationDeltaTime(now);

    auto& basic = pMsg->cam.camParameters.basicContainer;
    basic.stationType = TrafficParticipantType_passengerCar;
    double lat = 0.0;
    double lon = 0.0;
    double altitude = 0.0;
    basic.referencePosition.latitude = encodeValue(lat, LatitudeUnit_degree, -Latitude_unavailable + 1, Latitude_unavailable - 1);
    basic.referencePosition.longitude = encodeValue(lon, LongitudeUnit_degree, -Longitude_unavailable + 1, Longitude_unavailable - 1);
    basic.referencePosition.altitude.altitudeValue = encodeValue(altitude, AltitudeValueUnit_metre, AltitudeValue_negativeOutOfRange, AltitudeValue_postiveOutOfRange);
    basic.referencePosition.altitude.altitudeConfidence = AltitudeConfidence_unavailable;

    double pos_var_x = 1.0;
    double pos_var_y = 1.0;
    double pos_var_angle = 1.0;
    basic.referencePosition.positionConfidenceEllipse.semiMajorAxisLength =
        encodeConfidenceFromStdDev(std::sqrt(pos_var_x), SemiAxisLengthUnit_metre, SemiAxisLength_outOfRange);
    basic.referencePosition.positionConfidenceEllipse.semiMinorAxisLength =
        encodeConfidenceFromStdDev(std::sqrt(pos_var_y), SemiAxisLengthUnit_metre, SemiAxisLength_outOfRange);
    basic.referencePosition.positionConfidenceEllipse.semiMajorAxisOrientation =
        encodeWgsAngleNoCorrection(pos_var_angle);

    double heading = 1.0;
    double heading_var = 1.0;
    pMsg->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;
    auto& hf = pMsg->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
    // add offset to 0 being North, and invert the angle because it is defined in the opposite direction
    heading = std::fmod(-heading + M_PI_2, 2 * M_PI);
    if (heading < 0)
    {
      heading += 2 * M_PI;
    }
    hf.heading.headingValue = static_cast<int16_t>(heading * 180.0 / M_PI / HeadingValueUnit_degree);
    double s = std::sqrt(heading_var) * 180 / M_PI;
    hf.heading.headingConfidence =
        encodeConfidenceFromStdDev(s, HeadingConfidenceUnit_degree, HeadingConfidence_outOfRange);

    double vel = 1.0;
    double vel_var = 1.0;
    hf.speed.speedValue = encodeValue(vel, SpeedValueUnit_m_s, 0, SpeedValue_outOfRange);
    hf.speed.speedConfidence =
        encodeConfidenceFromStdDev(std::sqrt(vel_var), SpeedConfidenceUnit_m_s, SpeedConfidence_outOfRange);
    hf.driveDirection = (vel >= 0) ? DriveDirection_forward : DriveDirection_backward;

    double length = 1.0;
    double width = 1.0;
    hf.vehicleLength.vehicleLengthValue =
        encodeValue(length, VehicleLengthValueUnit_metre, 0, VehicleLengthValue_outOfRange);
    hf.vehicleLength.vehicleLengthConfidenceIndication = VehicleLengthConfidenceIndication_unavailable;
    hf.vehicleWidth = encodeValue(width, VehicleWidthUnit_metre, 0, VehicleWidth_outOfRange);

    double acc_long = 1.0;
    double acc_long_var = 1.0;
    hf.longitudinalAcceleration.value = encodeValue(acc_long,
                                                   AccelerationValueUnit_m_s_2,
                                                   AccelerationValue_negativeOutOfRange,
                                                   AccelerationValue_positiveOutOfRange);
    hf.longitudinalAcceleration.confidence =
        encodeConfidenceFromStdDev(std::sqrt(acc_long_var), AccelerationConfidenceUnit_m_s_2, AccelerationConfidence_outOfRange);

    hf.curvature.curvatureValue = CurvatureValue_unavailable;
    hf.curvature.curvatureConfidence = CurvatureConfidence_unavailable;
    hf.curvatureCalculationMode = CurvatureCalculationMode_unavailable;

    double yrt = 1.0 * 180 / M_PI;
    double yrt_var = 1.0;
    hf.yawRate.yawRateValue = encodeValue(
        yrt, YawRateValueUnit_degree_per_second_, YawRateValue_negativeOutOfRange, YawRateValue_positiveOutOfRange);
    hf.yawRate.yawRateConfidence = get_yawrate_confidence(std::sqrt(yrt_var));

    sendETSIMsg(&asn_DEF_CAM, et::ETSIMessageType::CAM, pMsg.get());
  }

protected:
  void handleCAM(const std::shared_ptr<const CAM>& msg, const et::BinaryETSIMessage &msg_bin) override
  {
    LOG_INF("Received a CAM from station ID " << msg_bin.station_id);

    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t reconstructed_time = et::GenerationDeltaTime2UnixTime(msg->cam.generationDeltaTime, now);
    LOG_INF("CAM time: " << reconstructed_time << ", delay: " << (now - reconstructed_time)*1e-6 << "ms");
  }
  void handleVAM(const std::shared_ptr<const VAM>& msg, const et::BinaryETSIMessage &msg_bin) override
  {
    LOG_INF("Received a VAM from station ID " << msg_bin.station_id);

    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t reconstructed_time = et::GenerationDeltaTime2UnixTime(msg->vam.generationDeltaTime, now);
    LOG_INF("VAM time: " << reconstructed_time << ", delay: " << (now - reconstructed_time)*1e-6 << "ms");
  }
  void handleCompleteCPM(StationId_t station_id, std::map<uint8_t, std::shared_ptr<const CollectivePerceptionMessage>> msgs, uint64_t time) override
  {
    LOG_INF("Received a CPM from station ID " << station_id);

    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG_INF("CPM time: " << time << ", delay: " << (now - time)*1e-6 << "ms");
  }
  void handleMCM(const std::shared_ptr<const MCM>& msg, const et::BinaryETSIMessage &msg_bin) override
  {
    LOG_INF("Received an MCM from station ID " << msg_bin.station_id);

    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t reconstructed_time = et::GenerationDeltaTime2UnixTime(msg->mcm.generationDeltaTime, now);
    LOG_INF("MCM time: " << time << ", delay: " << (now - reconstructed_time)*1e-6 << "ms");
  }
};

int main()
{
  mrm::v2x_amqp_connector_lib::_setLogLevel(aduulm_logger::LoggerLevel::Warn);
  mrm::v2x_etsi_asn1_lib::_setLogLevel(aduulm_logger::LoggerLevel::Warn);
  aduulm_logger::initLogger();
  aduulm_logger::setLogLevel(aduulm_logger::LoggerLevel::Debug);
  ETSIAMQPTransceiver transceiver{};

  uint32_t station_id = 101;
  // Set IP and port of broker here
  std::string url = "localhost:5672";
  // If set to empty string, transmitting messages is disabled
  std::string address_rx = "etsi";
  // If set to empty string, receiving messages is disabled
  std::string address_tx = "etsi";
  // User and password
  std::string user = "anonymous";
  std::string pw = "";

  // Filter queries (only applied for receiving messages (address_rx is not empty)
  // Examples for filter queries:
  //   Receive all messages: ""
  //   Receive all messages not from station ID 101: "NOT(station_id = 101)"
  //   Receive all CPM messages not from station ID 101: "NOT(station_id = 101) AND mid = 2049"
  //   Receive all CPM messages from station ID 42: "station_id = 42 AND mid = 2049"
  std::string filter_query = "";

  transceiver.connect(station_id, url, address_rx, address_tx, user, pw, filter_query);
  while (1) {
    // Transceiver has its own receiver thread, so messages will be automatically received in that thread.
    // The main thread can be used to send messages.
    // Make sure to use locks to protect from threading race conditions when accessing data from receiver callback handler functions.

    transceiver.sendDummyCAM();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
  }
}

// for aduulm_logger: This has to be part of the compile unit of the executable.
// Otherwise, undefined reference errors will result.
DEFINE_LOGGER_VARIABLES
