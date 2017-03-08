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
include_directories(SYSTEM ${glfw_INCLUDE_DIRS})
include_directories(SYSTEM ${glslang_INCLUDE_DIRS})
include_directories(SYSTEM ${VkHLF_INCLUDE_DIRS})

add_library(sga SHARED ${LIBSGA_SOURCES})
set_target_properties(sga PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
add_dependencies(sga VkHLF)

target_link_libraries(sga
  PRIVATE -Wl,--no-as-needed # This forces a runtime link dependency to libvulkan.so
  PUBLIC ${Vulkan_LIBRARIES}
  # Following libraries are static, so these get embedded into libsga.
  PRIVATE ${VkHLF_LIBRARIES} # static
  PRIVATE ${glfw_LIBRARIES}  # static
  PRIVATE Xrandr X11 Xcursor Xxf86vm Xinerama # dynamic
  PRIVATE -Wl,--whole-archive ${glslang_LIBRARIES} -Wl,--no-whole-archive # static
  )
