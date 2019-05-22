cmake_minimum_required(VERSION 3.9)

# list source files
set(SOURCE_FILES
	src/unittest_loom.cpp
	src/unittest_main.cpp
)

# add executable target
add_executable(unittest
	${SOURCE_FILES}
)

target_link_libraries(unittest
	PRIVATE
		MoustaphaSaad::loom
)

# add doctest folder
target_include_directories(unittest
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/doctest
)

# output directory
set_target_properties(unittest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)