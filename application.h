//
// Created by tim on 7/13/20.
//

#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application {
public:
    Application();

    ~Application();

    void run();

private:
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                 const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    bool checkDeviceExtensions(VkPhysicalDevice device);

    bool checkValidationLayerSupport();

    void createInstance();

    void createLogicalDevice();

    void createSurface();

    void getDeviceQueueIndices();

    std::vector<const char*> getRequiredExtensions() const;

    void initVulkan();

    void initWindow();

    bool isDeviceSuitable(VkPhysicalDevice device);

    VkPhysicalDevice pickPhysicalDevice();

    void setupDebugMessenger();

    const uint32_t WINDOW_WIDTH = 800;
    const uint32_t WINDOW_HEIGHT = 600;

    GLFWwindow *window;

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkSurfaceKHR surface;

    const std::array<const char*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    const std::array<const char*, 1> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
    };

    QueueFamilyIndices queueIndices;

    // Debug utilities
    VkDebugUtilsMessengerEXT debugMessenger;
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};