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
