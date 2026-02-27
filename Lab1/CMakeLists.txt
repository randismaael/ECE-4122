cmake_minimum_required(VERSION 3.10)
project(Lab1)

set(CMAKE_CXX_STANDARD 17)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(Lab1
    code/main.cpp
    code/ECE_Buzzy.cpp
    code/ECE_Enemy.cpp
    code/ECE_LaserBlast.cpp
)
target_link_libraries(Lab1 sfml-graphics sfml-window sfml-system)
