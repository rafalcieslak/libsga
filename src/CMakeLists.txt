add_definitions(
  --std=c++14
  )


if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(
    -Wall -Wextra
	-Werror
	-g
	)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

file(GLOB LIBSGA_SOURCES
  ./*.cpp
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp
  @ONLY)

include_directories(./include)
include_directories(SYSTEM ../include)

add_executable(libsga_test ${LIBSGA_SOURCES})
