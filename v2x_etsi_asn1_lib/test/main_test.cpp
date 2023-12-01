#include <gtest/gtest.h>
#include <aduulm_logger/aduulm_logger.hpp>

DEFINE_LOGGER_VARIABLES

int main(int argc, char** argv)
{
  aduulm_logger::initLogger(true);
  aduulm_logger::setLogLevel(aduulm_logger::LoggerLevels::Info);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
