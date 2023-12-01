#include <cstdint>
#include <string>

namespace mrm::v2x_etsi_asn1_lib
{
// Unix Time is always in nanoseconds since 1970 (UTC)
// ETSI time is always in milliseconds since 2004 (TAI)
uint64_t UnixTime2ETSITime(uint64_t unix_time);
uint64_t ETSITime2UnixTime(uint64_t etsi_time);
unsigned int UnixTime2GenerationDeltaTime(uint64_t unix_time);
uint64_t GenerationDeltaTime2UnixTime(unsigned int genDeltaTime, uint64_t now_unix_time);
uint64_t UnixTime2TaiTime(uint64_t unix_time);
uint64_t TaiTime2UnixTime(uint64_t tai_time);

}  // namespace mrm::v2x_etsi_asn1_lib
