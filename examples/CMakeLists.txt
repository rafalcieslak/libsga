include_directories(SYSTEM ../include)

if(WIN32)
  if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(
      /Wall /Yg
	  )
  endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
else(WIN32)
  if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(
      -Wall -Wextra
	  -g
	  )
  endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
endif(WIN32)


function(add_example NAME)
  if(${ARGC} EQUAL 1)
    set(REQUIRES "")
  else()
    set(REQUIRES ${ARGV})
    list(REMOVE_AT REQUIRES 0)
  endif()

  # Ensure all required dependencies are present
  foreach(REQ ${REQUIRES})
    if(NOT "${${REQ}_FOUND}")
      message("${REQ} not found, example ${NAME} will not be build.")
      return()
    endif()
  endforeach()

  foreach(REQ ${REQUIRES})
    list(APPEND REQ_INCLUDE_DIRS "${${REQ}_INCLUDE_DIRS}")
    list(APPEND REQ_LIBRARIES "${${REQ}_LIBRARIES}")
  endforeach()
  
  file(GLOB EXAMPLE_SOURCES
    ./${NAME}/*.cpp
    )
  file(GLOB EXAMPLE_SHADERS
    ./${NAME}/*.frag
    )
  set(TARGET_NAME sga-example-${NAME})
  include_directories(${REQ_INCLUDE_DIRS})
  add_executable(${TARGET_NAME} ${EXAMPLE_SOURCES})
  target_link_libraries(${TARGET_NAME} sga)
  target_link_libraries(${TARGET_NAME} ${REQ_LIBRARIES})
  # message("Linking ${TARGET_NAME} with ${REQ_LIBRARIES}")
  set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  if(EXAMPLE_SHADERS)
    add_custom_command(
	  TARGET ${TARGET_NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy ${EXAMPLE_SHADERS} "${PROJECT_BINARY_DIR}/examples/data/${NAME}/"
    )
  endif()
endfunction(add_example)

add_custom_target(
  copy-example-data ALL
  COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/examples/data ${CMAKE_BINARY_DIR}/examples/data
  )

add_example(triangle)
add_example(fragTest1)
add_example(sampler)
add_example(shadertoy)
add_example(cube GLM)
add_example(textureTarget)
add_example(twoOffscreenPipelines)
add_example(teapot GLM ASSIMP)
add_example(ssaa GLM ASSIMP)
add_example(deferred GLM ASSIMP)
add_example(unwrap-envmap)
