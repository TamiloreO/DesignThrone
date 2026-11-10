cmake_minimum_required(VERSION 3.20)
project(vxcore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_library(vxcore STATIC
  src/arith.cpp
  src/c_api.cpp
  src/time.cpp
  src/model.cpp
  src/project_io.cpp
  src/timeline.cpp
)

target_include_directories(vxcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
  target_compile_options(vxcore PRIVATE /W4 /permissive-)
else()
  target_compile_options(vxcore PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# The core must never depend on a platform framework. On Apple platforms this
# setting keeps it free of Objective-C and of automatic framework linking.
set_target_properties(vxcore PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
)

enable_testing()

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE vxcore)

add_test(NAME core_tests COMMAND core_tests)

# The C ABI is compiled as C here, not C++, so a pure C host is proven to work.
add_executable(c_api_smoke tests/c_api_smoke.c)
target_link_libraries(c_api_smoke PRIVATE vxcore)
set_target_properties(c_api_smoke PROPERTIES LINKER_LANGUAGE CXX)
add_test(NAME c_api_smoke COMMAND c_api_smoke)
