# Benchmark suite: compare gemma3:1b, gemma3:4b, gemma3:12b via gateway
# From project root: cmake -B build-bench -S scripts/benchmark-models && cmake --build build-bench
# Run: GATEWAY_URL=http://localhost:8080 API_KEY=xxx ./build-bench/benchmark_models_test

cmake_minimum_required(VERSION 3.14)
project(benchmark_models_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(HERMES_SRC "${PROJECT_ROOT}/src")

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(JsonCpp QUIET)
if(NOT JsonCpp_FOUND)
  find_path(JSONCPP_INCLUDE_DIR json/json.h
    PATHS /usr/include/jsoncpp /usr/include /usr/local/include)
  find_library(JSONCPP_LIBRARY jsoncpp
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /usr/lib /usr/local/lib)
  if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
    add_library(JsonCpp::JsonCpp INTERFACE IMPORTED)
    set_target_properties(JsonCpp::JsonCpp PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "${JSONCPP_LIBRARY}"
    )
  else()
    message(FATAL_ERROR "JsonCpp not found. Install libjsoncpp-dev (Linux) or jsoncpp via vcpkg (Windows).")
  endif()
endif()

add_executable(benchmark_models_test
  benchmark_models_test.cpp
  prompts.cpp
)
target_include_directories(benchmark_models_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${HERMES_SRC}"
)
target_link_libraries(benchmark_models_test PRIVATE
  GTest::gtest_main
  JsonCpp::JsonCpp
)

# Full suite: metrics, scoring, report-xxxx.json/csv/txt + gemma3:12b report-xxxx.md
add_executable(benchmark_suite_test
  benchmark_suite_test.cpp
  prompts.cpp
  scoring/expected_answers.cpp
  scoring/math_validator.cpp
  scoring/logic_validator.cpp
  scoring/score_utils.cpp
)
target_include_directories(benchmark_suite_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${HERMES_SRC}"
)
target_link_libraries(benchmark_suite_test PRIVATE
  GTest::gtest_main
  JsonCpp::JsonCpp
)
