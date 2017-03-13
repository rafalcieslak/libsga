include_directories(SYSTEM ../include)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(
    -Wall -Wextra
	-Werror
	-g
	)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

macro(add_example NAME)
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
endmacro(add_example)

add_example(greet)
add_example(image)
add_example(image2)
add_example(window)
add_example(pipeline)
add_example(vbo)
add_example(fragTest1)
add_example(shadertoy)
