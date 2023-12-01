#include <v2x_etsi_asn1_lib/time_conversions.h>
#include <gtest/gtest.h>
#include <date/date.h>
#include <date/tz.h>

namespace mrm::v2x_etsi_asn1_lib
{
using namespace date;
using namespace date::literals;
using namespace std::chrono_literals;

static void testConversionUNIX(uint64_t unix_time, unsigned int genDeltaTime)
{
  auto converted = UnixTime2GenerationDeltaTime(unix_time);
  ASSERT_EQ(converted, genDeltaTime);
  auto converted2 = GenerationDeltaTime2UnixTime(converted, unix_time + 10'000'000'000);
  ASSERT_EQ(converted2, unix_time);
}

TEST(TimeConversionTests, sanityTest)
{
  auto test_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(local_days{ January / 1 / 2004 }.time_since_epoch()).count();
  ASSERT_EQ(UnixTime2ETSITime(test_time), 0);
  ASSERT_EQ(ETSITime2UnixTime(0), test_time);
}

TEST(TimeConversionTests, testForwardBackward)
{
  auto unix_time = 1665064574'123'000'000;
  auto res = UnixTime2ETSITime(unix_time);
  auto unix_ref =
      std::chrono::duration_cast<std::chrono::nanoseconds>(local_days{ January / 1 / 2004 }.time_since_epoch()).count();
  auto res_without_leaps = (unix_time - unix_ref) / 1'000'000;
  ASSERT_EQ(res - res_without_leaps, 5'000);

  auto res2 = ETSITime2UnixTime(res);
  ASSERT_EQ(res2, unix_time);
}

TEST(TimeConversionTests, testForwardBackward2)
{
  auto unix_time = 1665054116'801'000'000;
  auto converted = UnixTime2GenerationDeltaTime(unix_time);
  auto converted2 = GenerationDeltaTime2UnixTime(converted, unix_time + 10'000'000'000);
  ASSERT_EQ(converted2, unix_time);
}

TEST(TimeConversionTests, testBoschReferenceValues)
{
  testConversionUNIX(1664975024'000'000'000ULL, 2824);
  testConversionUNIX(1663039020'000'000'000ULL, 63336);
  testConversionUNIX(1664961470'000'000'000ULL, 14776);
}

TEST(TimeConversionTests, testITSExampleValue)
{
  auto example =
      std::chrono::duration_cast<std::chrono::nanoseconds>(local_days{ January / 1 / 2007 }.time_since_epoch()).count();
  auto res = UnixTime2ETSITime(example);
  ASSERT_EQ(res, 94'694'401'000);
}

TEST(TimeConversionTests, testUULMReferenceValue)
{
  testConversionUNIX(1665054116'801'000'000ULL, 59209);
}

TEST(TimeConversionTests, testTaiConversions)
{
  auto unix_time = 1665054116'801'000'000;
  auto tai_time = UnixTime2TaiTime(unix_time);

  // at the timestamp above, there is a difference of 37s
  ASSERT_EQ(tai_time - unix_time, 37'000'000'000);

  auto unix_time_ = TaiTime2UnixTime(tai_time);
  ASSERT_EQ(unix_time_ - unix_time, 0);
}

}  // namespace mrm::v2x_etsi_asn1_lib
