#pragma once

#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "buffer.h"
#include "gltfloader.h"
#include "scene.h"
#include "shaderloader.h"
#include "swapchain.h"
#include "texture.h"
#include "ui.h"
#include "vertex.h"

namespace blitz {
class Renderer {
public:
    Renderer();

    ~Renderer();

    void run();

private:
    struct QueueFamilyIndices {
        uint32_t graphicsFamilyIndex;
        uint32_t computeFamilyIndex;
        uint32_t presentFamilyIndex;
    };

    static VkResult
    CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                 const VkAllocationCallbacks *pAllocator,
                                 VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                               "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData) {

        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto app = reinterpret_cast<Renderer *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

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

    VkCommandBuffer beginSingleTimeCommands();

    bool checkDeviceExtensions(VkPhysicalDevice device);

    bool checkValidationLayerSupport();

    void cleanupSwapchain();

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    Buffer::BufferContext createBufferContext();

    void createCommandBuffers();

    void createCommandPool();

    void createDepthResources();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void createDescriptorSets();

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                     VkDeviceMemory &imageMemory);

    void createIndexBuffer();

    void createInstance();

    void createLogicalDevice();

    void createMsaaResources();

    void createOffscreenFramebuffer();

    void createOffscreenRenderPass();

    void createPipelines();

    void createRenderPass();

    VkShaderModule createShaderModule(const std::vector<char> &shaderCode);

    void createSurface();

    void createSwapchain();

    Swapchain::SwapchainContext createSwapchainContext();

    void createSyncObjects();

    void createTextureImage();

    void createTextureSampler();

    UserInterface::UIContext createUIContext();

    void createUniformBuffers();

    void createVertexBuffer();

    void drawFrame();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void generateMipMaps(const Texture &tex);

    void getDeviceQueueIndices();

    std::vector<const char *> getRequiredExtensions() const;

    inline bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void initUI();

    void initVulkan();

    void initWindow();

    bool isDeviceSuitable(VkPhysicalDevice device);

    void loadScene();

    VkFormat pickDepthFormat();

    static VkSampleCountFlagBits pickMaxUsableSampleCount(VkPhysicalDevice device);

    uint32_t pickMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkPhysicalDevice pickPhysicalDevice();

    VkFormat pickSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    void recreateSwapchain();

    void setupDebugMessenger();

    void transitionImageLayout(const Image &image, VkImageLayout oldLayout, VkImageLayout newLayout);

    void updateCameraBuffers(const size_t &bufferIdx);

    void updateLightBuffers(const size_t &bufferIdx);

    void updateUniformBuffer(size_t bufferIdx);

    uint32_t windowWidth = 1920;
    uint32_t windowHeight = 1080;

    GLFWwindow *window = {};

    VkInstance instance = {};
    VkPhysicalDevice physicalDevice = {};
    VkDevice logicalDevice = {};

    VkQueue graphicsQueue = {};
    VkQueue presentQueue = {};

    VkSurfaceKHR surface = {};

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VertexBuffer vertexBuffer = {};
    IndexBuffer vertIndexBuffer = {};

    std::vector<UniformBuffer> cameraBuffers;
    std::vector<UniformBuffer> lightBuffers;

    Texture texture;
    VkSampler textureSampler = {};

    Image depthImage;
    Image msaaImage;

    VkRenderPass renderPass = {};

    VkPipeline graphicsPipeline = {};
    VkPipelineLayout graphicsPipelineLayout = {};

    VkCommandPool commandPool = {};
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    VkDescriptorPool descriptorPool = {};
    VkDescriptorSetLayout descriptorSetLayout = {};
    std::vector<VkDescriptorSet> descriptorSets = {};

    Image normalImage;
    VkRenderPass offscreenRenderPass;
    VkFramebuffer offscreenFrameBuffer;

    VkSampler colorSampler = {};

    Swapchain swapchain;

    UserInterface ui;

    Scene scene;

    bool framebufferResized = false;

    std::vector<const char *> requiredExtensions;

    const std::array<const char *, 1> validationLayers = {
            "VK_LAYER_KHRONOS_validation",
    };

    const std::array<const char *, 1> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    QueueFamilyIndices queueIndices = {};

    // Debug utilities
    VkDebugUtilsMessengerEXT debugMessenger = {};
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};
} // namespace blitz