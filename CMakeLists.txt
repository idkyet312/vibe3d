cmake_minimum_required(VERSION 3.10)
project(vibe3d)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set PhysX paths
if(NOT DEFINED PHYSX_ROOT_DIR)
    set(PHYSX_ROOT_DIR "/home/bas/Downloads/PhysX-107.0-physx-5.6.0/physx" CACHE PATH "Path to PhysX SDK root directory")
endif()
set(PHYSX_INCLUDE_DIR "${PHYSX_ROOT_DIR}/include")
set(PHYSX_LIB_DIR "${PHYSX_ROOT_DIR}/bin/linux.x86_64/release")

# Print the absolute paths
message(STATUS "PhysX Root Directory: ${PHYSX_ROOT_DIR}")
message(STATUS "PhysX Include Directory: ${PHYSX_INCLUDE_DIR}")
message(STATUS "PhysX Library Directory: ${PHYSX_LIB_DIR}")

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Add source files
set(SOURCES
    test.cpp
    src/glad.c
)

# Create executable
add_executable(vibe3d ${SOURCES})

# Include directories
target_include_directories(vibe3d PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${PHYSX_INCLUDE_DIR}
    include
)

# Link directories
link_directories(${PHYSX_LIB_DIR})

# Link libraries
target_link_libraries(vibe3d PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    ${PHYSX_LIB_DIR}/libPhysXExtensions_static_64.a
    ${PHYSX_LIB_DIR}/libPhysX_static_64.a
    ${PHYSX_LIB_DIR}/libPhysXPvdSDK_static_64.a
    ${PHYSX_LIB_DIR}/libPhysXCommon_static_64.a
    ${PHYSX_LIB_DIR}/libPhysXFoundation_static_64.a
    ${PHYSX_LIB_DIR}/libPhysXCooking_static_64.a
    pthread
    dl
)

# Copy shader files to build directory
file(COPY 
    ${CMAKE_SOURCE_DIR}/vertex.glsl 
    ${CMAKE_SOURCE_DIR}/fragment.glsl
    DESTINATION ${CMAKE_BINARY_DIR})

# Find Threads
find_package(Threads REQUIRED)
target_link_libraries(vibe3d PRIVATE Threads::Threads)

# Add shader files to the build
configure_file(${CMAKE_SOURCE_DIR}/floor_vertex.glsl ${CMAKE_BINARY_DIR}/floor_vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/floor_fragment.glsl ${CMAKE_BINARY_DIR}/floor_fragment.glsl COPYONLY)

# Add CPack for packaging (optional future step)
# include(InstallRequiredSystemLibraries)
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
# set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
# include(CPack) 