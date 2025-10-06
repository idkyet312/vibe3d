cmake_minimum_required(VERSION 3.20)
project(vibe3d VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Try to find Vulkan (optional)
find_package(Vulkan)

# Add source files
set(SOURCES
    main.cpp
    GraphicsManager.cpp
    MaterialSystem.cpp
    PhysicsManager.cpp
    InputManager.cpp
)

# Add Vulkan sources if available
if(Vulkan_FOUND)
    list(APPEND SOURCES
        src/vulkan/VulkanRenderer.cpp
        src/vulkan/VulkanDevice.cpp
        src/vulkan/VulkanSwapChain.cpp
        src/vulkan/VulkanPipeline.cpp
        src/vulkan/VulkanBuffer.cpp
        src/vulkan/VulkanImage.cpp
        src/vulkan/VulkanDescriptor.cpp
        src/vulkan/ForwardPlusRenderer.cpp
        src/vulkan/modules/GeometryManager.cpp
        src/vulkan/ImGuiManager.cpp
        src/vulkan/PostProcessManager.cpp
    )
    message(STATUS "? Vulkan SDK found - building with Vulkan support")
else()
    message(STATUS "? Vulkan SDK not found - building OpenGL-only version")
    message(STATUS "   Download Vulkan SDK from: https://vulkan.lunarg.com/")
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
    glad::glad
    imgui::imgui
    nlohmann_json::nlohmann_json
)

# Add Vulkan support if available
if(Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE VIBE3D_VULKAN_SUPPORT)
endif()

# Platform-specific definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(Vulkan_FOUND AND WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

# Copy OpenGL shaders
file(GLOB GLSL_SHADERS "*.glsl" "*.comp")
foreach(SHADER ${GLSL_SHADERS})
    configure_file(${SHADER} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()

# Compile Vulkan shaders if available
if(Vulkan_FOUND AND Vulkan_GLSLC_EXECUTABLE)
    file(GLOB VULKAN_SHADERS "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")
    if(VULKAN_SHADERS)
        foreach(SHADER ${VULKAN_SHADERS})
            get_filename_component(SHADER_NAME ${SHADER} NAME)
            set(SPIRV_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
            add_custom_command(
                OUTPUT ${SPIRV_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
                COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER}" -o "${SPIRV_OUTPUT}"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
                VERBATIM
            )
            list(APPEND SPIRV_SHADERS ${SPIRV_OUTPUT})
        endforeach()
        
        add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_SHADERS})
        add_dependencies(${PROJECT_NAME} compile_shaders)
    endif()
endif()

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4 
        /permissive-
        /Zc:__cplusplus
        /EHsc
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Enable C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Build summary
message(STATUS "========================================")
message(STATUS "Vibe3D Configuration:")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Vulkan Support: ${Vulkan_FOUND}")
if(Vulkan_FOUND)
    message(STATUS "  Vulkan Version: ${Vulkan_VERSION}")
    message(STATUS "  GLSL Compiler: ${Vulkan_GLSLC_EXECUTABLE}")
endif()
message(STATUS "========================================")