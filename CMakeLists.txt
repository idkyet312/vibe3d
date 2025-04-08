cmake_minimum_required(VERSION 3.10)
project(Vibe3DBox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenGL REQUIRED)
# Use PkgConfig to find GLFW
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)

# Add glad source file
add_executable(${PROJECT_NAME} test.cpp src/glad.c) 

# Use include directories provided by PkgConfig and add 'include' for glad headers
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${OpenGL_INCLUDE_DIR} 
    ${GLFW3_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR}/include # Use absolute path
)

# Use libraries provided by PkgConfig and link other libs
target_link_libraries(${PROJECT_NAME} PUBLIC 
    OpenGL::GL 
    ${GLFW3_LIBRARIES} 
)

# On some systems you might need to link pthread explicitly for glfw
find_package(Threads REQUIRED)
# pkg-config for glfw3 usually handles linking Threads::Threads

# Copy shader files to the build directory (optional, but convenient)
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${SHADER_FILE}
                       $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADER_NAME}
                       COMMENT "Copying ${SHADER_NAME} to build directory")
endforeach() 