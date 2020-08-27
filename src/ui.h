#pragma once

#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "swapchain.h"

class UserInterface {
public:
    struct UIContext {
        GLFWwindow *window;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkQueue graphicsQueue;
        uint32_t imageCount;
        uint32_t graphicsFamilyIndex;
        Swapchain swapchain;
    };

    UserInterface() = default;

    void cleanup();

    void draw();

    void init(const UIContext &rendererCtx);

    VkCommandBuffer recordCommands(uint32_t bufferIdx, VkExtent2D extent2D);

    void recreate(const UIContext &ctx);

private:
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool);

    void cleanupResources();

    void createUICommandBuffers();

    void createUICommandPool(VkCommandPoolCreateFlags flags);

    void createUIDescriptorPool();

    void createUIFramebuffers();

    void createUIRenderPass();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool cmdPool) const;

    UIContext context = {};

    VkRenderPass renderPass = {};
    VkDescriptorPool descriptorPool = {};
    VkCommandPool commandPool = {};

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> framebuffers;
};