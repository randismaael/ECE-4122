cmake_minimum_required(VERSION 3.16)
project(Lab2Life CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)
find_package(SFML 2.5 COMPONENTS system window graphics QUIET)

if(NOT SFML_FOUND AND TARGET sfml-graphics)
  set(SFML_FOUND TRUE)
endif()

add_executable(Lab2 main.cpp)

if(SFML_FOUND)
  target_link_libraries(Lab2 PRIVATE sfml-graphics sfml-window sfml-system)
else()
  message(WARNING "SFML not found; building HEADLESS fallback.")
  target_compile_definitions(Lab2 PRIVATE HEADLESS)
endif()

if(OpenMP_CXX_FOUND)
  target_link_libraries(Lab2 PRIVATE OpenMP::OpenMP_CXX)
endif()

set_target_properties(Lab2 PROPERTIES
  BUILD_RPATH "$ORIGIN/../lib"
  INSTALL_RPATH "$ORIGIN/../lib"
)
