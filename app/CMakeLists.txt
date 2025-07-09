# MongooseVKDemo CMake file

# Find all source files
FILE(GLOB_RECURSE APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")

# Create the executable
add_executable(MongooseVKDemo ${APP_SOURCES})

# Link the library to the executable
target_link_libraries(MongooseVKDemo PRIVATE MongooseVK)

# Set include directories for the application (if needed)
target_include_directories(MongooseVKDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
