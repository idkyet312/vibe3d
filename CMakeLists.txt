cmake_minimum_required(VERSION 3.16)
project(vibe3d)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Add source files
set(SOURCES
    main.cpp
    GraphicsManager.cpp
    MaterialSystem.cpp
    PhysicsManager.cpp
    InputManager.cpp
    src/glad.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw
)

# Copy shaders to build directory
file(GLOB SHADER_FILES "*.glsl" "*.comp" "*.vert" "*.frag")
foreach(SHADER ${SHADER_FILES})
    configure_file(${SHADER} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()