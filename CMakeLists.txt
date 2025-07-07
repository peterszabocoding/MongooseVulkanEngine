#---------------------------------------------------------------------------------------------------
# CMake Build Configuration for the Ray Tracing Weekend Series
#---------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.30.2)

project(RTWeekend LANGUAGES C CXX)

# Set to C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setup project dependencies
message(STATUS "Looking for Vulkan...")
find_package(Vulkan REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# Alright, no errors? Tell me what you found!
message("VULKAN_FOUND " ${VULKAN_FOUND})
message("Vulkan_LIBRARIES: " ${Vulkan_LIBRARIES})
message("Vulkan_LIBRARY: " ${Vulkan_LIBRARY})
message("Vulkan_INCLUDE_DIRS: " ${Vulkan_INCLUDE_DIRS})
message("Vulkan_GLSLC_EXECUTABLE: " ${Vulkan_GLSLC_EXECUTABLE})
message("Vulkan_glslang_FOUND : " ${Vulkan_glslang_FOUND})
message("Vulkan_shaderc_combined_FOUND : " ${Vulkan_shaderc_combined_FOUND})
message("Vulkan_shaderc_combined_LIBRARY: " ${Vulkan_shaderc_combined_LIBRARY})

include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0.1
)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.91.8
)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG v1.15.2
)

FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

message(STATUS "Fetching GLFW...")
FetchContent_MakeAvailable(glfw)

message(STATUS "Fetching GLM...")
FetchContent_MakeAvailable(glm)

message(STATUS "Fetching ImGui...")
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching fmt...")
FetchContent_MakeAvailable(spdlog)

message(STATUS "Fetching Tracy...")
FetchContent_MakeAvailable(tracy)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(SPDLOG_INCLUDE_DIR ${spdlog_SOURCE_DIR}/include)

set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/)
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)

add_library(
        imgui STATIC
        ${IMGUI_SOURCES}
)

# Source
set(EXTERNAL
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

set(SOURCE_ONE_WEEKEND
        ./src/main.cpp

        ./src/application/window.cpp
        ./src/application/application.cpp

        ./src/renderer/renderer.cpp
        ./src/renderer/camera.cpp
        ./src/renderer/shader_cache.cpp
        ./src/renderer/vulkan/vulkan_skybox.cpp

        ./src/renderer/vulkan/vulkan_renderer.cpp
        ./src/renderer/vulkan/vulkan_pipeline.cpp
        ./src/renderer/vulkan/vulkan_device.cpp
        ./src/renderer/vulkan/vulkan_image.cpp
        ./src/renderer/vulkan/vulkan_swapchain.cpp
        ./src/renderer/vulkan/vulkan_framebuffer.cpp
        ./src/renderer/vulkan/vulkan_renderpass.cpp
        ./src/renderer/vulkan/vulkan_buffer.cpp
        ./src/renderer/vulkan/vulkan_mesh.cpp
        ./src/renderer/vulkan/vulkan_descriptor_set_layout.cpp
        ./src/renderer/vulkan/vulkan_descriptor_pool.cpp
        ./src/renderer/vulkan/vulkan_descriptor_writer.cpp
        ./src/renderer/vulkan/vulkan_material.cpp
        ./src/renderer/vulkan/vulkan_texture.cpp
        ./src/renderer/vulkan/vulkan_cube_map_texture.cpp
        ./src/renderer/vulkan/vulkan_shadow_map.cpp
        ./src/renderer/vulkan/vulkan_reflection_probe.cpp
        ./src/renderer/vulkan/vulkan_shader_compiler.cpp
        ./src/renderer/vulkan/vulkan_gbuffer.cpp
        ./src/renderer/vulkan/imgui_vulkan.cpp

        ./src/renderer/vulkan/pass/render_pass.cpp
        ./src/renderer/vulkan/pass/shadow_map_pass.cpp
        ./src/renderer/vulkan/pass/present_pass.cpp
        ./src/renderer/vulkan/pass/gbufferPass.cpp
        ./src/renderer/vulkan/pass/infinite_grid_pass.cpp
        ./src/renderer/vulkan/pass/skybox_pass.cpp

        ./src/renderer/vulkan/pass/post_processing/ssao_pass.cpp

        ./src/renderer/vulkan/lighting/irradiance_map_generator.cpp
        ./src/renderer/vulkan/lighting/reflection_probe_generator.cpp

        ./src/resource/resource_manager.cpp
        ./src/resource/loaders/obj_loader.cpp
        ./src/resource/loaders/gltf_loader.cpp

        ./src/memory/resource_pool.cpp

        ./src/input/camera_controller.cpp
        ./src/input/input.cpp

        ./src/util/log.cpp
        ./src/util/filesystem.cpp
)

include_directories("src/")
include_directories("vendor/")

# Specific compiler flags below. We're not going to add options for all possible compilers, but if
# you're new to CMake (like we are), the following may be a helpful example if you're using a
# different compiler or want to set different compiler options.

message(STATUS "Compiler ID: " ${CMAKE_CXX_COMPILER_ID})
message(STATUS "Release flags: " ${CMAKE_CXX_FLAGS_RELEASE})
message(STATUS "Debug flags: " ${CMAKE_CXX_FLAGS_DEBUG})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /wd #### - Disable warning
    # /we #### - treat warning as error
    add_compile_options("/W4")      # Enable level-4 warnings
    add_compile_options("/we 4265") # Class has virtual functions, but its non-trivial destructor is not virtual
    add_compile_options("/we 5204") # Class has virtual functions, but its trivial destructor is not virtual
    add_compile_options("/wd 4100") # unreferenced formal parameter
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wnon-virtual-dtor) # Class has virtual functions, but its destructor is not virtual
    add_compile_options(-Wreorder) # Data member will be initialized after [other] data member
    add_compile_options(-Wmaybe-uninitialized) # Variable improperly initialized
    add_compile_options(-Wunused-variable) # Variable is defined but unused
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wnon-virtual-dtor) # Class has virtual functions, but its destructor is not virtual
    add_compile_options(-Wreorder) # Data member will be initialized after [other] data member
    add_compile_options(-Wsometimes-uninitialized) # Variable improperly initialized
    add_compile_options(-Wunused-variable) # Variable is defined but unused
elseif (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    add_compile_options(-Wnon-virtual-dtor) # Class has virtual functions, but its destructor is not virtual
    add_compile_options(-Wreorder) # Data member will be initialized after [other] data member
    add_compile_options(-Wsometimes-uninitialized) # Variable improperly initialized
    add_compile_options(-Wunused-variable) # Variable is defined but unused
endif ()

# Global definitions
add_compile_definitions(NDEBUG)

set(_ITERATOR_DEBUG_LEVEL 0)

FILE(GLOB_RECURSE HEADERS "src/**.h")

if (APPLE)
    add_compile_definitions(PLATFORM_MACOS)
    add_executable(inOneWeekend ${EXTERNAL} ${HEADERS} ${SOURCE_ONE_WEEKEND} ${SOURCE_ONE_WEEKEND_APPLE})
    target_link_libraries( inOneWeekend PRIVATE ${Vulkan_shaderc_combined_LIBRARY})
endif (APPLE)

if (WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
    add_executable(inOneWeekend ${EXTERNAL} ${HEADERS} ${SOURCE_ONE_WEEKEND} ${SOURCE_ONE_WEEKEND_WINDOWS})
    target_link_libraries( inOneWeekend PRIVATE debug C:/VulkanSDK/1.4.309.0/Lib/shaderc_combinedd.lib optimized ${Vulkan_shaderc_combined_LIBRARY})
endif (WIN32)

target_compile_definitions(inOneWeekend PRIVATE GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_XYZW_ONLY GLM_FORCE_QUAT_DATA_XYZW GLM_FORCE_QUAT_CTOR_XYZW)
target_compile_definitions(inOneWeekend PRIVATE ENABLE_ASSERTS)

set_target_properties(glfw PROPERTIES FOLDER "Dependencies")
set_target_properties(glm PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui PROPERTIES FOLDER "Dependencies")
set_target_properties(spdlog PROPERTIES FOLDER "Dependencies")

target_include_directories(inOneWeekend PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(inOneWeekend PRIVATE ${IMGUI_INCLUDE_DIR})
target_include_directories(inOneWeekend PRIVATE ${SPDLOG_INCLUDE_DIR})

target_link_libraries(inOneWeekend PRIVATE ${Vulkan_LIBRARIES})

target_link_libraries(inOneWeekend PRIVATE glfw)
target_link_libraries(inOneWeekend PRIVATE glm)
target_link_libraries(inOneWeekend PRIVATE imgui)
target_link_libraries(inOneWeekend PRIVATE spdlog)
target_link_libraries(inOneWeekend PUBLIC TracyClient)
