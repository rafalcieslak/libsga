file(GLOB LIBSGA_SOURCES
  ./*.cpp
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp
  @ONLY)

include_directories(./include)
include_directories(SYSTEM ../include)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})

add_library(sga SHARED ${LIBSGA_SOURCES})
set_target_properties(sga PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_link_libraries(sga ${Vulkan_LIBRARIES})
