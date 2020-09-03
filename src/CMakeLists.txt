SET(SRCS main.cpp
        renderer.cpp
        shaderloader.h
        swapchain.cpp
        swapchain.h
        ui.cpp
        ui.h
        vertex.h
        gltfloader.h
        gltfloader.cpp)

add_executable(renderer ${SRCS})

add_subdirectory(imgui)

target_include_directories(renderer PUBLIC imgui cgltf)

target_link_libraries(renderer glfw Vulkan::Vulkan glm::glm)