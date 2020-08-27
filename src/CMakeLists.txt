SET(SRCS main.cpp
        renderer.cpp shaderloader.h swapchain.cpp swapchain.h ui.cpp ui.h)

add_executable(renderer ${SRCS})

add_subdirectory(imgui)

target_include_directories(renderer PUBLIC imgui)

target_link_libraries(renderer glfw Vulkan::Vulkan glm::glm)