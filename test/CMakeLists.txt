cmake_minimum_required(VERSION 3.14)
project(spsp_test)

Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

include_directories(
  "../include/common"
  "../priv_include/common"
  "include"
)
file(GLOB srcs CONFIGURE_DEPENDS
  "../src/common/*.cpp"
  "src/*.cpp"
  "tests/*.cpp"
)
add_executable(spsp_test ${srcs})

set_property(TARGET spsp_test PROPERTY CXX_STANDARD 17)

target_link_libraries(spsp_test PRIVATE Catch2::Catch2WithMain)
