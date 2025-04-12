#---------------------------------------------------------------------------------------------------
# CMake Build Configuration for the Ray Tracing Weekend Series
#---------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.30.2)

project(RTWeekend LANGUAGES C CXX)

# Set to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Setup project dependencies
message(STATUS "Looking for Vulkan...")
find_package(Vulkan REQUIRED)

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
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 11.1.4
)

message(STATUS "Fetching GLFW...")
FetchContent_MakeAvailable(glfw)

message(STATUS "Fetching GLM...")
FetchContent_MakeAvailable(glm)

message(STATUS "Fetching ImGui...")
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching fmt...")
FetchContent_MakeAvailable(fmt)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

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

        ./src/renderer/shader.cpp
        ./src/renderer/renderer.cpp
        ./src/renderer/camera.cpp
        ./src/renderer/vulkan/vulkan_renderer.cpp
        ./src/renderer/vulkan/vulkan_pipeline.cpp
        ./src/renderer/vulkan/vulkan_device.cpp
        ./src/renderer/vulkan/vulkan_image.cpp
        ./src/renderer/vulkan/vulkan_shader.cpp
        ./src/renderer/vulkan/vulkan_swapchain.cpp
        ./src/renderer/vulkan/vulkan_renderpass.cpp
        ./src/renderer/vulkan/vulkan_buffer.cpp
        ./src/renderer/vulkan/vulkan_mesh.cpp
        ./src/renderer/vulkan/imgui_vulkan.cpp
)

set(SOURCE_ONE_WEEKEND_APPLE
        src/platform/macos/macos_application.cpp
        src/platform/macos/macos_window.cpp
)

set(SOURCE_ONE_WEEKEND_WINDOWS
        src/platform/windows/windows_application.cpp
        src/platform/windows/windows_window.cpp
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

FILE(GLOB_RECURSE HEADERS "src/**.h")

if (APPLE)
    add_compile_definitions(PLATFORM_MACOS)

    add_executable(inOneWeekend ${EXTERNAL} ${HEADERS} ${SOURCE_ONE_WEEKEND} ${SOURCE_ONE_WEEKEND_APPLE})
endif (APPLE)

if (WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
    add_executable(inOneWeekend ${EXTERNAL} ${HEADERS} ${SOURCE_ONE_WEEKEND} ${SOURCE_ONE_WEEKEND_WINDOWS})
endif (WIN32)


set_target_properties(glfw PROPERTIES FOLDER "Dependencies")
set_target_properties(glm PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui PROPERTIES FOLDER "Dependencies")
set_target_properties(fmt PROPERTIES FOLDER "Dependencies")

target_include_directories(inOneWeekend PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(inOneWeekend PRIVATE ${IMGUI_INCLUDE_DIR})

target_link_libraries(inOneWeekend PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(inOneWeekend PRIVATE glfw)
target_link_libraries(inOneWeekend PRIVATE glm)
target_link_libraries(inOneWeekend PRIVATE imgui)
target_link_libraries(inOneWeekend PRIVATE fmt)
