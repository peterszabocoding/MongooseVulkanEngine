#---------------------------------------------------------------------------------------------------
# CMake Build Configuration for the Ray Tracing Weekend Series
#---------------------------------------------------------------------------------------------------

cmake_minimum_required ( VERSION 3.30.2 )


project ( RTWeekend LANGUAGES C CXX )


# Set to C++17
set ( CMAKE_CXX_STANDARD          17 )
set ( CMAKE_CXX_STANDARD_REQUIRED ON )
set ( CMAKE_CXX_EXTENSIONS        OFF )


message(STATUS "Fetching GLFW...")

include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# Source

set ( EXTERNAL
  #src/external/stb_image.h
  ./external/glad/src/glad.c

  ./external/imgui/imgui.cpp
  ./external/imgui/imgui_demo.cpp
  ./external/imgui/imgui_draw.cpp
  ./external/imgui/imgui_tables.cpp
  ./external/imgui/imgui_widgets.cpp

  ./external/imgui/backends/imgui_impl_glfw.cpp
  ./external/imgui/backends/imgui_impl_opengl3.cpp
)

set ( SOURCE_ONE_WEEKEND
  ./src/main.cpp

  ./src/application/window.cpp
  ./src/application/application.cpp
  
  ./src/platform/macos/macosApplication.cpp
  ./src/platform/macos/macosWindow.cpp

  ./src/renderer/shader.cpp
  ./src/renderer/metalShader.cpp
  ./src/renderer/metalRenderer.cpp
)

include_directories(external/glad/include)
include_directories(external/imgui)
include_directories(src)

# Specific compiler flags below. We're not going to add options for all possible compilers, but if
# you're new to CMake (like we are), the following may be a helpful example if you're using a
# different compiler or want to set different compiler options.

message (STATUS "Compiler ID: " ${CMAKE_CXX_COMPILER_ID})
message (STATUS "Release flags: " ${CMAKE_CXX_FLAGS_RELEASE})
message (STATUS "Debug flags: " ${CMAKE_CXX_FLAGS_DEBUG})

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
endif()

# Executables
add_executable(inOneWeekend ${EXTERNAL} ${SOURCE_ONE_WEEKEND})
target_link_libraries (inOneWeekend PRIVATE glfw)

# Add OpenGL on Apple platforms
if(APPLE)
  # Library definition
  include_directories(external/metal-cmake/metal-cpp)
  include_directories(external/metal-cmake/metal-cpp-extensions)
  add_subdirectory(external/metal-cmake) 

  add_compile_definitions(PLATFORM_MACOS)
  
  target_link_libraries (inOneWeekend PRIVATE METAL_CPP)
  target_link_libraries (inOneWeekend PRIVATE "-framework OpenGL")
endif(APPLE)
