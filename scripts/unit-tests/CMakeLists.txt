# Unit tests for enterprise security plugins (RF-32/33/34/35)
# Build: cmake -B build -S scripts/unit-tests && cmake --build build
# Run:   ./build/test_security_plugins  (or build\test_security_plugins.exe on Windows)

cmake_minimum_required(VERSION 3.14)
project(test_security_plugins LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate the gateway src root (two levels up from this file)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(HERMES_SRC "${PROJECT_ROOT}/src")

# ── GoogleTest ─────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── JsonCpp ────────────────────────────────────────────────────────────────────
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
      INTERFACE_LINK_LIBRARIES      "${JSONCPP_LIBRARY}"
    )
  else()
    message(FATAL_ERROR
      "JsonCpp not found. Install libjsoncpp-dev (Linux) or jsoncpp via vcpkg (Windows).")
  endif()
endif()

# ── Test target ────────────────────────────────────────────────────────────────
add_executable(test_security_plugins test_security_plugins.cpp)

target_include_directories(test_security_plugins PRIVATE
  "${HERMES_SRC}"
  "${HERMES_SRC}/plugins/enterprise"
)

target_link_libraries(test_security_plugins PRIVATE
  GTest::gtest_main
  JsonCpp::JsonCpp
)

include(GoogleTest)
gtest_discover_tests(test_security_plugins)
