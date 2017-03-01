# Version 3.5 is required for CMAKE_EXPORT_COMPILE_COMMANDS
cmake_minimum_required(VERSION 3.5)

IF(DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

# Version information
execute_process(
  COMMAND git describe --tags --always --abbrev=0
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE PROJECT_VERSION_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
execute_process(
  COMMAND git describe --tags --long --dirty --always
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
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

# Import custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Doxygen)
find_package(Vulkan)

if(NOT Vulkan_FOUND)
  message(SEND_ERROR "Unable to find Vulkan installed on this system.")
endif()

# Global definitions
add_definitions(--std=c++14)

if((${CMAKE_BUILD_TYPE} MATCHES "Release"))
  add_definitions( -O3 )
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions( -Wall -Wextra -Werror -g)
endif()

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(examples)
