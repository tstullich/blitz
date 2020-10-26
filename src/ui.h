#pragma once

#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "camera.h"
#include "swapchain.h"

namespace blitz {
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
        Camera camera;
        // TODO Refactor this
        float yFov;
        float aspectRatio;
        float zNear;
        float zFar;
    };

    struct UIOptions {
        bool rotate;
        glm::vec3 camPosition;
        glm::vec3 camLookAt;
        float yFov;
        float aspectRatio;
        float zNear;
        float zFar;
    };

    UserInterface() = default;

    void cleanup();

    void draw();

    inline UIOptions getOptions() const { return options; }

    void init(const UIContext &rendererCtx);

    VkCommandBuffer recordCommands(uint32_t bufferIdx, VkExtent2D extent2D);

    void recreate(const UIContext &ctx);

private:
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool) const;

    void cleanupResources();

    void createCommandBuffers();

    void createCommandPool(VkCommandPoolCreateFlags flags);

    void createDescriptorPool();

    void createFramebuffers();

    void createRenderPass();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool cmdPool) const;

    UIContext context = {};
    UIOptions options = { .rotate = false };

    VkRenderPass renderPass = {};
    VkDescriptorPool descriptorPool = {};
    VkCommandPool commandPool = {};

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> framebuffers;
};
} // namespace blitz