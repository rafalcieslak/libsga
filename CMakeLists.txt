cmake_minimum_required(VERSION 3.0)

IF(DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

# Version information
execute_process(
  COMMAND git describe --tags --always --abbrev=0
  OUTPUT_VARIABLE PROJECT_VERSION_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
execute_process(
  COMMAND git describe --tags --long --dirty --always
  OUTPUT_VARIABLE PROJECT_VERSION_LONG
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
string(SUBSTRING ${PROJECT_VERSION_SHORT} 1 -1 PROJECT_VERSION_VERYSHORT)
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  string(CONCAT PROJECT_VERSION_SHORT ${PROJECT_VERSION_SHORT} "-debug")
  string(CONCAT PROJECT_VERSION_LONG  ${PROJECT_VERSION_LONG}  "-debug")
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

message(STATUS "Build mode: ${CMAKE_BUILD_TYPE}, version: ${PROJECT_VERSION_LONG}")

# Project init
project(
  libsga
  VERSION "${PROJECT_VERSION_VERYSHORT}"
  LANGUAGES CXX C
  )

add_subdirectory(src)
