#---------------------------------------------------------------------------------------------------
# CMake Build Configuration for MongooseVK Vulkan renderer
#---------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.30.2)

project(MongooseVulkanRenderer LANGUAGES C CXX)

# Set to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

# Add the library and application subdirectories
add_subdirectory(engine)
add_subdirectory(app)
