include_directories(SYSTEM ../include)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(
    -Wall -Wextra
	-Werror
	-g
	)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

macro(add_test NAME)
  file(GLOB EXAMPLE_SOURCES
    ./${NAME}/*.cpp
    )
  add_executable(${NAME} ${EXAMPLE_SOURCES})
  target_link_libraries(${NAME} sga)
  set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/examples)
endmacro(add_test)

add_test(greet)
