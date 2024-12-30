# CMakeLists.txt for RHI
cmake_minimum_required(VERSION 3.10)
project(ForgeRHI)

set(BUILD_TESTS OFF CACHE BOOL "Disable building tests" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)

if(MSVC)
    add_compile_options(/wd4819)  # Disable warning about non-UTF8 source files (example)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3") # Set warning level to 3 (lower level)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Fetch fmt
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

# Set up the RHI library
add_library(ForgeRHI STATIC)

# Set the source files for ForgeRHI
set(RHI_SOURCES
    src/Forge.cpp
    src/ForgeSwapchain.cpp
    src/ForgeBuffer.cpp
    src/ForgeImage.cpp
    src/ForgeRenderPass.cpp
    src/ForgeShader.cpp
    src/ForgeBindingList.cpp
    src/ForgeDescriptorSetManager.cpp
    src/ForgeDynamicMemory.cpp
    src/ForgeFrame.cpp
    src/ForgeCommandBufferManager.cpp
    src/ForgeDeferredQueue.cpp
    # Add other RHI source files here
)

# Include the headers for RHI
set(RHI_HEADERS
    include/Forge.h
    include/ForgeLogger.h
    include/ForgeUtils.h
    include/ForgeSwapchain.h
    include/ForgeBuffer.h
    include/ForgeImage.h
    include/ForgeRenderPass.h
    include/ForgeShader.h
    include/ForgeBindingList.h
    include/ForgeDescriptorSetManager.h
    include/ForgeDynamicMemory.h
    include/ForgeFrame.h
    include/ForgeCommandBufferManager.h
    include/ForgeDeferredQueue.h
    # Add other public headers here
)

# Add the source files to the library target
target_sources(ForgeRHI PRIVATE ${RHI_SOURCES} ${RHI_HEADERS})

# Add include directories for ForgeRHI
target_include_directories(ForgeRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Vulkan_LIBRARIES} # Vulkan include directories
        ${shaderc_SOURCE_DIR}/include
)

# Link Vulkan to ForgeRHI
target_link_libraries(ForgeRHI
    PUBLIC
        Vulkan::Vulkan
        fmt::fmt
        $ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib
        $ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib
)

# Set C++ standard
set_property(TARGET ForgeRHI PROPERTY CXX_STANDARD 17)

# Set output directory for binaries
set_target_properties(ForgeRHI PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)