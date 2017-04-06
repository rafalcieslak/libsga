file(GLOB LIBSGA_SOURCES
  ./*.cpp
  )

include_directories(./include)
include_directories(SYSTEM ../include)
include_directories(SYSTEM ${vulkan_INCLUDE_DIRS})
include_directories(SYSTEM ${glfw_INCLUDE_DIRS})
include_directories(SYSTEM ${glslang_INCLUDE_DIRS})
include_directories(SYSTEM ${VkHLF_INCLUDE_DIRS})

add_library(sga SHARED ${LIBSGA_SOURCES})
set_target_properties(sga PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sga PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_link_libraries(sga
  PRIVATE -Wl,--no-as-needed # This forces a runtime link dependency to libvulkan.so
  PRIVATE ${Vulkan_LIBRARIES}
  # Following libraries are static, so these get embedded into libsga.
  PRIVATE glslang # static
  PRIVATE SPIRV # static
  PRIVATE glfw  # static
  PRIVATE VkHLF-static # static
  # PRIVATE Xrandr X11 Xcursor Xxf86vm Xinerama # dynamic
  )
