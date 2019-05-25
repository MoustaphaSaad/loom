cmake_minimum_required(VERSION 3.9)

# list the header files
set(HEADER_FILES
	include/loom/Exports.h
	include/loom/Group.h
	include/loom/Loom.h
	include/loom/MapReduce.h
)

# list the source files
set(SOURCE_FILES
	src/loom/Group.cpp
	src/loom/Loom.cpp
)

# add library target
add_library(loom
	${HEADER_FILES}
	${SOURCE_FILES}
)

add_library(MoustaphaSaad::loom ALIAS loom)

# make it reflect the same structure as the one on disk
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})


# Windows specfic flags to enable W variations of winapi
if(WIN32)
	target_compile_definitions(loom
		PRIVATE
			OS_WINDOWS=1
	)
	target_compile_options(loom
		PRIVATE
			/WX /W4
	)
elseif(UNIX)
	target_compile_definitions(loom
		PRIVATE
			OS_LINUX=1
	)
	target_compile_options(loom
		PRIVATE
			-Wall -Werror
	)
endif()


# enable C++17
# disable any compiler specifc extensions
# add d suffix in debug mode
target_compile_features(loom PUBLIC cxx_std_17)
set_target_properties(loom PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
)

# define debug macro
target_compile_definitions(loom PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

# generate exports header file
include(GenerateExportHeader)
generate_export_header(loom
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/loom/Exports.h
)

# list include directories
target_include_directories(loom
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# link to mn
target_link_libraries(loom
	PUBLIC
		MoustaphaSaad::mn
)
