include(ExternalProject)

# Workaround against ExternalProject sporiously failing to find git
find_package(Git)

# Note: As of current CMake release, there is no way to ask GIT_SUBMODULES to
# update NO submodules. Therefore we use a custom git download command to
# clone VkHLF without initializing submodules we won't even use.
ExternalProject_Add( glslang
  PREFIX glslang
  GIT_REPOSITORY https://github.com/rafalcieslak/glslang
  GIT_TAG libsga
  DOWNLOAD_COMMAND rm -rf glslang && git clone -b libsga --single-branch --depth 1 https://github.com/rafalcieslak/glslang
  )

ExternalProject_Get_Property(glslang INSTALL_DIR)
set(glslang_LIBRARY_DIR ${INSTALL_DIR}/src/glslang-build/install/lib)
set(glslang_INCLUDE_DIR ${INSTALL_DIR}/src)

ExternalProject_Add( VkHLF
  DEPENDS glslang
  PREFIX vkhlf
  GIT_REPOSITORY https://github.com/rafalcieslak/VkHLF
  GIT_TAG libsga
  GIT_SUBMODULES "3rdparty"
  DOWNLOAD_COMMAND rm -rf VkHLF && git clone -b libsga --single-branch --depth 1 https://github.com/rafalcieslak/VkHLF
  CMAKE_ARGS -DBUILD_VKCPP_SAMPLES=OFF -DBUILD_SUBMODULES=OFF -DEXTRA_INCLUDE_DIRS=${glslang_INCLUDE_DIR}
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/vkhlf/src/VkHLF/vkhlf ${CMAKE_CURRENT_BINARY_DIR}/install/include/vkhlf &&
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/vkhlf/src/VkHLF-build/bin/release ${CMAKE_CURRENT_BINARY_DIR}/install/lib
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/vkhlf/src/VkHLF-build/lib/release ${CMAKE_CURRENT_BINARY_DIR}/install/lib
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install
  )

# Export VkHLF configuration
ExternalProject_Get_Property(VkHLF INSTALL_DIR)
set(VkHLF_INCLUDE_DIRS ${INSTALL_DIR}/include PARENT_SCOPE)
set(VkHLF_LIBRARIES ${INSTALL_DIR}/lib/libVkHLF.a PARENT_SCOPE)

# Export glslang configuration
set(glslang_LIBRARIES
  ${glslang_LIBRARY_DIR}/libglslang.a
  ${glslang_LIBRARY_DIR}/libOGLCompiler.a
  ${glslang_LIBRARY_DIR}/libOSDependent.a
  ${glslang_LIBRARY_DIR}/libSPIRV.a
  ${glslang_LIBRARY_DIR}/libHLSL.a
  PARENT_SCOPE)
