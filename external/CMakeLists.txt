# Custom vulkan header distributed with source
set(vulkan_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vulkan)
set(vulkan_INCLUDE_DIRS ${vulkan_INCLUDE_DIRS} PARENT_SCOPE)

# GLFW 3
message("- Configuring GLFW3")
OPTION(GLFW_BUILD_DOCS OFF)
OPTION(GLFW_BUILD_EXAMPLES OFF)
OPTION(GLFW_BUILD_TESTS OFF)
add_subdirectory(glfw)
set(glfw_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)
set(glfw_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/glfw/src/libglfw3.a PARENT_SCOPE)

# glslang
message("- Configuring glslang")
add_subdirectory(glslang)
set(glslang_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
set(glslang_INCLUDE_DIRS ${glslang_INCLUDE_DIRS} PARENT_SCOPE)
set(glslang_LIBRARIES
  ${CMAKE_CURRENT_BINARY_DIR}/glslang/glslang/libglslang.a
  ${CMAKE_CURRENT_BINARY_DIR}/glslang/OGLCompilersDLL/libOGLCompiler.a
  ${CMAKE_CURRENT_BINARY_DIR}/glslang/glslang/OSDependent/Unix/libOSDependent.a
  ${CMAKE_CURRENT_BINARY_DIR}/glslang/SPIRV/libSPIRV.a
  ${CMAKE_CURRENT_BINARY_DIR}/glslang/hlsl/libHLSL.a
  PARENT_SCOPE)

# vkhlf
message("- Configuring VkHLF")
set(BUILD_VKCPP_SAMPLES OFF)
set(BUILD_SUBMODULES OFF)
set(EXTRA_INCLUDE_DIRS ${glslang_INCLUDE_DIRS} ${vulkan_INCLUDE_DIRS})
add_subdirectory(vkhlf)
set(VkHLF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vkhlf PARENT_SCOPE)
set(VkHLF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/vkhlf/lib/libVkHLF.a PARENT_SCOPE)
