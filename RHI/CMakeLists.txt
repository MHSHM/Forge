# CMakeLists.txt for RHI
cmake_minimum_required(VERSION 3.10)
project(ForgeRHI)

# Set up the RHI library
add_library(ForgeRHI STATIC)

# Set the source files for ForgeRHI
set(RHI_SOURCES
    src/Swapchain.cpp
    # Add other RHI source files here
)

# Include the headers for RHI
set(RHI_HEADERS
    include/Swapchain.h
    # Add other public headers here
)

# Add the source files to the library target
target_sources(ForgeRHI PRIVATE ${RHI_SOURCES} ${RHI_HEADERS})

# Add include directories for ForgeRHI
target_include_directories(ForgeRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set C++ standard
set_property(TARGET ForgeRHI PROPERTY CXX_STANDARD 17)

# Set output directory for binaries
set_target_properties(ForgeRHI PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)