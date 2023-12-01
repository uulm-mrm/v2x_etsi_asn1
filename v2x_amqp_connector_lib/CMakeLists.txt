cmake_minimum_required(VERSION 3.10)
project(v2x_amqp_connector_lib)

# Get aduulm cmake macros for coloring, package_version extraction and find_aduulm_package()
find_package(aduulm_cmake_tools REQUIRED)
get_version_string_from_package_xml(.)
show_build_info("library")
setup_git_hooks()


find_aduulm_package(aduulm_logger_lib REQUIRED)
find_package(ProtonCpp REQUIRED)

set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Path to installed include directory")
set(LIB_INSTALL_DIR lib/ CACHE STRING "Path to installed lib directory")
set(BIN_INSTALL_DIR bin/ CACHE STRING "Path to installed bin directory")

# TARGETS

add_library(${PROJECT_NAME} SHARED
	src/v2x_amqp_connector_lib.cpp
	src/logger_setup.cpp
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)

foreach(TARGET ${PROJECT_NAME})
  target_link_libraries(${TARGET}
    PUBLIC
      Proton::cpp
  )

  # Link aduulm targets
  link_aduulm_package_targets(TARGET ${TARGET}
    ACCESS PUBLIC
    PACKAGE_TARGETS aduulm_logger_lib::aduulm_logger_lib
  )

  target_compile_features(${TARGET} PUBLIC cxx_std_17)
  target_compile_definitions(${TARGET} PUBLIC "-D${package_name}_VERSION=\"${package_version}\"")
  target_compile_definitions(${TARGET} PRIVATE -DROS_PACKAGE_NAME="${PROJECT_NAME}")
endforeach()


# INSTALLATION (for libraries)
set(PACKAGE_LIBRARY_VERSION ${package_version})

# Create package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${PACKAGE_LIBRARY_VERSION}"
    COMPATIBILITY AnyNewerVersion
)

# Install files for all targets
install(TARGETS ${PROJECT_NAME} # (add additional targets here)
    EXPORT ${PROJECT_NAME}Targets # store targets in variable
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Development
    RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime
    PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Development
    BUNDLE DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime
)

# Create package config file (to enable find_package())
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/config.cmake.in" # this file defines the include dirs and libs and may need to be adapted for your package/library
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)

# Create targets file with all targets (enables to link against single targets)
install(EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
)

# install CMake config and version file
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME})

# install header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${INCLUDE_INSTALL_DIR})
