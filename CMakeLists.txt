cmake_minimum_required(VERSION 3.10)
project(Vibe3DGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)

# GLM is typically header-only, no find_package needed if installed via libglm-dev

# Add executable sources (including GLAD)
add_executable(${PROJECT_NAME} test.cpp src/glad.c) 

# Include directories (OpenGL, GLFW from pkg-config, GLAD headers)
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${OpenGL_INCLUDE_DIR} 
    ${GLFW3_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR}/include # Absolute path for GLAD/KHR
)

# Link libraries (OpenGL, GLFW from pkg-config)
target_link_libraries(${PROJECT_NAME} PUBLIC 
    OpenGL::GL 
    ${GLFW3_LIBRARIES} 
)

# Find Threads (pkg-config for glfw3 might handle this, but keep for safety)
find_package(Threads REQUIRED)
# target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads) # Often redundant with pkg-config

# Copy shader files to the build directory
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${SHADER_FILE}
                       $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADER_NAME}
                       COMMENT "Copying ${SHADER_NAME} to build directory")
endforeach() 

# Add shader files to the build
configure_file(${CMAKE_SOURCE_DIR}/vertex.glsl ${CMAKE_BINARY_DIR}/vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/fragment.glsl ${CMAKE_BINARY_DIR}/fragment.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/floor_vertex.glsl ${CMAKE_BINARY_DIR}/floor_vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/floor_fragment.glsl ${CMAKE_BINARY_DIR}/floor_fragment.glsl COPYONLY)

# Add CPack for packaging (optional future step)
# include(InstallRequiredSystemLibraries)
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
# set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
# include(CPack) 