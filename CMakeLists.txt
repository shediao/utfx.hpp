cmake_minimum_required(VERSION 3.15)
project(
  utfx
  VERSION "0.1"
  LANGUAGES CXX)

set("PROJECT_DESCRIPTION" "A header-only utf8,utf16,utf32 encoder&decoder")
set("PROJECT_HOMEPAGE_URL" "https://github.com/shediao/utfx.hpp")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(UTFX_BUILD_TESTS "Set to ON to build tests" ON)
else()
  option(UTFX_BUILD_TESTS "Set to OFF to build tests" OFF)
endif()

add_library(utfx INTERFACE)

target_include_directories(
  utfx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include>)

target_compile_features(utfx INTERFACE cxx_std_20)
add_library(utfx::utfx ALIAS utfx)

if(UTFX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(
  TARGETS utfx
  EXPORT utfx-targets
  INCLUDES
  DESTINATION include)

install(
  DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")

install(
  EXPORT utfx-targets
  FILE utfx-targets.cmake
  NAMESPACE utfx::
  DESTINATION lib/cmake/utfx)
