cmake_minimum_required(VERSION 3.10)
project(uav_gl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    main.cpp
    ECE_UAV.cpp
)

# Header files
set(HEADERS
    ECE_UAV.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    OpenGL::GLU
    GLUT::GLUT
    Threads::Threads
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})
