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
# Trim the leading "v"
string(SUBSTRING ${PROJECT_VERSION_SHORT} 1 -1 PROJECT_VERSION_SHORT)
string(SUBSTRING ${PROJECT_VERSION_LONG} 1 -1 PROJECT_VERSION_LONG)
set(PROJECT_VERSION_VERYSHORT ${PROJECT_VERSION_SHORT})
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

# TODO: Only the library is required, not the header, since we bring a custom version.
find_package(Vulkan) # manually required
find_package(Git REQUIRED)

find_package(Doxygen) # optional (for doc generation)

find_package(GLM) # optional (for some example programs)
find_package(assimp) # optional (for some example programs)
find_package(Freetype) # optional (for some example programs)
find_package(Eigen3) # optional (for some example programs)

if(NOT Vulkan_FOUND)
  message(SEND_ERROR "Unable to find Vulkan installed on this system.")
endif()

# This subdirectory must be processed before others, as it sets various
# variables for building with external libraries.
add_subdirectory(external)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DLIBSGA_EXPORTS)

# Global definitions
if(WIN32)
  if((${CMAKE_BUILD_TYPE} MATCHES "Release"))
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP")
    add_definitions( /O3 )
  endif()
  if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP")
    add_definitions( /Yg)
  endif()
else(WIN32)
  if((${CMAKE_BUILD_TYPE} MATCHES "Release"))
    add_definitions( -O3 )
  endif()
  if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions( -Wall -Wextra -g)
  endif()
endif(WIN32)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/sga/config.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/sga/config.hpp
  @ONLY)

# Release targets
if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
  set(PACKAGING_SCRIPT "${PROJECT_SOURCE_DIR}/release/release-windows.sh")
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  set(PACKAGING_SCRIPT "${PROJECT_SOURCE_DIR}/release/release-linux.sh")
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

if(DOXYGEN_FOUND)
add_custom_target(
  release
  COMMAND sh ${PACKAGING_SCRIPT} ${PROJECT_VERSION_LONG}
)
add_dependencies(
  release
  sga
  doc
)
endif(DOXYGEN_FOUND)


add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(examples)
