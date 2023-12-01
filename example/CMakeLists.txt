cmake_minimum_required(VERSION 3.10)
project(v2x_example)

set(aduulm_cmake_tools_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../aduulm_cmake_tools/no_framework)
add_subdirectory(../../../aduulm_logger/library build/aduulm_logger)
add_subdirectory(../v2x_amqp_connector_lib build/v2x_amqp_connector_lib)
add_subdirectory(../v2x_etsi_asn1_lib build/v2x_etsi_asn1_lib)

add_executable(${PROJECT_NAME}
  example.cpp
)
target_link_libraries(${PROJECT_NAME}
  v2x_etsi_asn1_lib::v2x_etsi_asn1_lib
)
