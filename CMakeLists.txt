cmake_minimum_required(VERSION 3.16)

project(blitz)

set(CMAKE_CXX_STANDARD 14)

# Adding different compiler flags for debug and release
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3")

# Installation directory
set(BLITZ_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(src)
add_subdirectory(shaders)