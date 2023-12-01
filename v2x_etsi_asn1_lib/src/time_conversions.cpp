#include "v2x_etsi_asn1_lib/time_conversions.h"
#include <ctime>
#include <date/date.h>
#include <date/tz.h>
#include <aduulm_logger/aduulm_logger.hpp>

namespace mrm::v2x_etsi_asn1_lib
{
/// Modulus for the generationTimeDelta calculation in ITS messages
static constexpr unsigned int ITS_modulus_ = 65536;

using namespace date;
using namespace date::literals;
using namespace std::chrono_literals;
static const auto etsi_start_utc = clock_cast<utc_clock>(local_days{ January / 1 / 2004 });

static const std::chrono::system_clock::time_point unix_epoch_10{ std::chrono::nanoseconds(-10'000'000'000) };
static const auto unix_epoch_tai = clock_cast<tai_clock>(unix_epoch_10);

// Unix Time is always in nanoseconds since 1970 (without leap seconds)
// ETSI time is always in milliseconds since 2004 (includes leap seconds)

uint64_t UnixTime2ETSITime(uint64_t unix_time)
{
  std::chrono::system_clock::time_point unix_time_pt{ std::chrono::nanoseconds(unix_time) };
  auto utc_time_pt = clock_cast<utc_clock>(unix_time_pt);
  auto diff = utc_time_pt - etsi_start_utc;
  auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
  return diff_ms;
}

uint64_t ETSITime2UnixTime(uint64_t etsi_time)
{
  utc_clock::time_point utc_time_pt{ std::chrono::milliseconds(etsi_time) + etsi_start_utc };
  auto unix_time_pt = clock_cast<std::chrono::system_clock>(utc_time_pt);

  return std::chrono::duration_cast<std::chrono::nanoseconds>((unix_time_pt).time_since_epoch()).count();
}

unsigned int UnixTime2GenerationDeltaTime(uint64_t unix_time)
{
  uint64_t time_ITS = UnixTime2ETSITime(unix_time);
  return time_ITS % ITS_modulus_;
}

uint64_t GenerationDeltaTime2UnixTime(unsigned int genDeltaTime, uint64_t now_unix_time)
{
  auto now_ITS = UnixTime2ETSITime(now_unix_time);
  unsigned int genDeltaTime2 = UnixTime2GenerationDeltaTime(now_unix_time);
  if (std::abs(static_cast<int64_t>(genDeltaTime) - static_cast<int64_t>(genDeltaTime2)) < ITS_modulus_ / 2)
  {
    return ETSITime2UnixTime(now_ITS - genDeltaTime2 + genDeltaTime);
  }
  return ETSITime2UnixTime(now_ITS - genDeltaTime2 - ITS_modulus_ + genDeltaTime);
}

uint64_t UnixTime2TaiTime(uint64_t unix_time)
{
  std::chrono::system_clock::time_point unix_time_pt{ std::chrono::nanoseconds(unix_time) };
  auto tai_time_pt = clock_cast<tai_clock>(unix_time_pt);
  return std::chrono::duration_cast<std::chrono::nanoseconds>(tai_time_pt - unix_epoch_tai).count();
}

uint64_t TaiTime2UnixTime(uint64_t tai_time)
{
  tai_clock::time_point tai_time_pt{ std::chrono::nanoseconds(tai_time + unix_epoch_tai.time_since_epoch().count()) };
  auto unix_time_pt = clock_cast<std::chrono::system_clock>(tai_time_pt);
  return std::chrono::duration_cast<std::chrono::nanoseconds>(unix_time_pt.time_since_epoch()).count();
}

}  // namespace mrm::v2x_etsi_asn1_lib
