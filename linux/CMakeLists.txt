cmake_minimum_required(VERSION 3.14)
project(spsp VERSION 0.4.1 DESCRIPTION "Simple publish subscribe protocol")

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

file(GLOB lib_srcs CONFIGURE_DEPENDS
  "../src/common/*.cpp"
  "../src/linux/*.cpp"
)
file(GLOB lib_includes CONFIGURE_DEPENDS
  "../include/common/*.hpp"
  "../include/linux/*.hpp"
)

# SPSP shared library
add_library(spsp SHARED ${lib_srcs})
set_target_properties(spsp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(spsp PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/common>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/linux>")
target_include_directories(spsp PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../priv_include/common>"
)
find_package(Threads REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(spsp PRIVATE Threads::Threads eclipse-paho-mqtt-c::paho-mqtt3as)
install(TARGETS spsp
  EXPORT spspTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES ${lib_includes} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Export CMake config
install(EXPORT spspTargets
  FILE spspConfig.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/spsp"
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/spspConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/spspConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/spsp"
)

# ESP-NOW bridge binary
add_executable(spsp_bridge_espnow bridge_espnow.cpp)
target_link_libraries(spsp_bridge_espnow PRIVATE spsp Threads::Threads)
install(TARGETS spsp_bridge_espnow DESTINATION "${CMAKE_INSTALL_BINDIR}")
