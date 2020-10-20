#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

class Swapchain {
public:
    struct SwapchainContext {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkCommandPool commandPool;
        VkSurfaceKHR surface;
        uint32_t graphicsQueueIndex;
        uint32_t presentQueueIndex;
        uint32_t extentWidth;
        uint32_t extentHeight;
    };

    struct SwapchainConfiguration {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    Swapchain() = default;

    void cleanup();

    void init(const SwapchainContext &context);

    void initFramebuffers(const VkRenderPass &renderPass, const VkImageView &depthImageView, const VkImageView &msaaImageView);

    inline size_t getFramebufferSize() const { return swapchainFramebuffers.size(); }

    inline VkExtent2D getExtent() const { return swapchainExtent; }

    inline VkFramebuffer getFramebuffer(const size_t &idx) { return swapchainFramebuffers[idx]; }

    inline uint32_t getImageCount() const { return imageCount; }

    inline VkFormat getImageFormat() const { return swapchainImageFormat; }

    inline size_t getImagesSize() const { return swapchainImages.size(); };

    inline size_t getImageViewsSize() const { return swapchainImageViews.size(); }

    inline VkImageView getImageView(const size_t &idx) const { return swapchainImageViews[idx]; }

    inline VkSwapchainKHR getSwapchain() const { return swapchain; }

    static SwapchainConfiguration querySwapchainSupport(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface);

private:
    void createImageViews();

    void createSwapchain();

    static VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities, uint32_t width, uint32_t height);

    static VkPresentModeKHR pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes);

    static VkSurfaceFormatKHR pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);

    SwapchainContext ctx = {};

    VkSwapchainKHR swapchain = {};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent = {};
    VkFormat swapchainImageFormat = {};
    std::vector<VkFramebuffer> swapchainFramebuffers;

    uint32_t imageCount = 0;
};