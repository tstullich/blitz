//
// Created by tim on 7/13/20.
//

#pragma once

#include <cstring>
#include <iostream>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class Application {
public:
    Application();

    ~Application();

    void run();

private:
    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void createInstance();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void initWindow();

    void initVulkan();

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    void setupDebugMessenger();

    const uint32_t WINDOW_WIDTH = 800;
    const uint32_t WINDOW_HEIGHT = 600;

    GLFWwindow *window;

    VkInstance instance;

    //VkPhysicalDevice physicalDevice;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};