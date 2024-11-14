# CMakeLists.txt for Playground
cmake_minimum_required(VERSION 3.10)
project(ForgePlayground)

# Set up the playground executable
add_executable(ForgePlayground)

# Set the source files for the playground
set(PLAYGROUND_SOURCES
    src/main.cpp
    # Add other playground source files here
)

# Add the source files to the playground target
target_sources(ForgePlayground PRIVATE ${PLAYGROUND_SOURCES})

# Link the RHI library to the playground
target_link_libraries(ForgePlayground PRIVATE ForgeRHI)

# Set C++ standard
set_property(TARGET ForgePlayground PROPERTY CXX_STANDARD 17)

# Include the RHI directory
target_include_directories(ForgePlayground
    PRIVATE
        ${CMAKE_SOURCE_DIR}/RHI/include
)

# Set output directory for binaries
set_target_properties(ForgePlayground PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
