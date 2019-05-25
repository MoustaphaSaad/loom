cmake_minimum_required(VERSION 3.9)

project(loom VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

add_subdirectory(mn/mn)
add_subdirectory(loom)
add_subdirectory(unittest)