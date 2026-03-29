# E2E tests - build with CMake (Windows or when gtest is not installed)
# Run from project root: cmake -B build-e2e -S scripts/e2e-core-with-plugins && cmake --build build-e2e
# Or from this dir:     cmake -B build -S . && cmake --build build

cmake_minimum_required(VERSION 3.14)
project(e2e_core_plugins_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Project root: when configuring from scripts/e2e-core-with-plugins, root is ../..
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

add_executable(e2e_core_plugins_test e2e_core_plugins_test.cpp)
target_include_directories(e2e_core_plugins_test PRIVATE
  "${HERMES_SRC}"
)
target_link_libraries(e2e_core_plugins_test PRIVATE
  GTest::gtest_main
  JsonCpp::JsonCpp
)
