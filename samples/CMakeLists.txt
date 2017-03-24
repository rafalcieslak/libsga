cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 OLD)

if(NOT DEFINED BUILD_VKCPP_SAMPLES)
  SET( BUILD_VKCPP_SAMPLES ON  CACHE BOOL "Build the VkCpp based samples" )
endif()

if( BUILD_VKCPP_SAMPLES )
  FILE (GLOB linkunits ${CMAKE_CURRENT_SOURCE_DIR}/*)

  FOREACH( linkunit ${linkunits} )
    if( IS_DIRECTORY ${linkunit} )
      if( EXISTS ${linkunit}/CMakeLists.txt )
        string( REGEX REPLACE "^.*/([^/]*)$" "\\1" LINK_NAME ${linkunit} )
        add_subdirectory( ${LINK_NAME} )
      endif()
    endif()
  ENDFOREACH( linkunit ${linkunits} )
endif()


