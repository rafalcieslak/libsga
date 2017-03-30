include_directories(SYSTEM ../include)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(
    -Wall -Wextra
	-g
	)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

function(add_example NAME)
  if(${ARGC} EQUAL 1)
    SET(REQUIRES "")
  else()
    SET(REQUIRES ${ARGV1})
  endif()

  # Ensure all required dependencies are present
  foreach(REQ ${REQUIRES})
    if(NOT "${${REQ}_FOUND}")
      message("${REQ} not found, example ${NAME} will not be build.")
      return()
    endif()
  endforeach()

  # TODO: Gather required resources for header dirs and link targets.
  
  file(GLOB EXAMPLE_SOURCES
    ./${NAME}/*.cpp
    )
  file(GLOB EXAMPLE_SHADERS
    ./${NAME}/*.frag
    )
  add_executable(${NAME} ${EXAMPLE_SOURCES})
  target_link_libraries(${NAME} sga)
  set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/examples)
  if(EXAMPLE_SHADERS)
    add_custom_command(
	  TARGET ${NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy ${EXAMPLE_SHADERS} "${PROJECT_BINARY_DIR}/examples/"
    )
  endif()
endfunction(add_example)

add_custom_target(
  copy-example-data ALL
  COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/examples/data ${CMAKE_BINARY_DIR}/data
  )
  
add_example(greet)
add_example(image)
add_example(image2)
add_example(window)
add_example(pipeline)
add_example(vbo)
add_example(fragTest1)
add_example(sampler)
add_example(shadertoy)
add_example(cube GLM)
add_example(textureTarget)
add_example(twoOffscreenPipelines)
