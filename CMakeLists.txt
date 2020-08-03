cmake_minimum_required(VERSION 3.16)
project(renderer)

set(CMAKE_CXX_STANDARD 14)

add_executable(renderer main.cpp application.cpp application.h)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

target_link_libraries(renderer glfw Vulkan::Vulkan)