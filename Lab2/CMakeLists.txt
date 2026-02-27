cmake_minimum_required(VERSION 3.16)
project(ConwayLifeSuperbuild CXX)

# Where to place the final binaries/libraries (matches PACE steps)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)

# Default to Release for performance
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- Build SFML from the source directory at ./SFML ---
# Trim features we don't need on the cluster to speed up build
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE)

add_subdirectory(SFML)   # Expect the SFML source tree in ./SFML

# --- Build the Lab2 target ---
add_subdirectory(Lab2)
