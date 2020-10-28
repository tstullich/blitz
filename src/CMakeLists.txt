SET(SRCS main.cpp
        renderer.cpp
        shaderloader.h
        swapchain.cpp
        swapchain.h
        ui.cpp
        ui.h
        vertex.h
        gltfloader.h
        gltfloader.cpp
        buffer.cpp
        buffer.h
        camera.h
        texture.h
        texture.cpp
        image.h
        image.cpp
        scene.cpp
        scene.h
        light.h)

add_executable(${PROJECT_NAME} ${SRCS})

add_subdirectory(imgui)

target_include_directories(${PROJECT_NAME} PUBLIC imgui tinygltf)

target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan glm::glm)

install(TARGETS ${PROJECT_NAME} DESTINATION ${BLITZ_INSTALL_BIN_DIR})