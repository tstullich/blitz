//
// Created by tim on 8/26/20.
//

#include "swapchain.h"

void Swapchain::cleanup() {
    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(ctx.logicalDevice, framebuffer, nullptr);
    }

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(ctx.logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(ctx.logicalDevice, swapchain, nullptr);
}

void Swapchain::init(const SwapchainContext &context) {
    ctx = context;
    createSwapchain();
    createImageViews();
}

void Swapchain::initFramebuffers(const VkRenderPass &renderPass, const VkImageView &depthImageView,
                                 const VkImageView &msaaImageView) {
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        // We need to attach an image view to the frame buffer for presentation purposes
        // as well as a view for the depth stencil. This also needs to match the order of the attachments
        // as defined in the Render Pass.
        std::array<VkImageView, 3> attachments = { msaaImageView, depthImageView, swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx.logicalDevice, &framebufferInfo, nullptr,
                                &swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create framebuffer!");
        }
    }
}

void Swapchain::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // We do not enable mip-mapping
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(ctx.logicalDevice, &createInfo, nullptr,
                              &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create image views!");
        }
    }
}

void Swapchain::createSwapchain() {
    SwapchainConfiguration configuration = querySwapchainSupport(ctx.physicalDevice, ctx.surface);

    VkSurfaceFormatKHR surfaceFormat = pickSwapchainSurfaceFormat(configuration.surfaceFormats);
    VkExtent2D extent = pickSwapchainExtent(configuration.capabilities, ctx.extentWidth, ctx.extentHeight);
    VkPresentModeKHR presentMode = pickSwapchainPresentMode(configuration.presentModes);

    uint32_t minImageCount = configuration.capabilities.minImageCount + 1;
    if (configuration.capabilities.maxImageCount > 0 && minImageCount > configuration.capabilities.maxImageCount) {
        // In case we are exceeding the maximum capacity for swap chain images we reset the value
        minImageCount = configuration.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = ctx.surface;
    swapchainCreateInfo.minImageCount = minImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO Change this later to setup for compute

    // Check if the graphics and present queues are the same and setup
    // sharing of the swap chain accordingly
    uint32_t indices[] = {ctx.graphicsQueueIndex, ctx.presentQueueIndex};
    if (ctx.graphicsQueueIndex == ctx.presentQueueIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = indices;
    }

    swapchainCreateInfo.preTransform = configuration.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create swap chain!");
    }

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    // Store the handles to the swap chain images for later use
    vkGetSwapchainImagesKHR(ctx.logicalDevice, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.logicalDevice, swapchain, &imageCount, swapchainImages.data());
}

VkExtent2D Swapchain::pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities, uint32_t width, uint32_t height) {
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width, height};
        actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
                                      std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
                                       std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkPresentModeKHR Swapchain::pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes) {
    // Look for triple-buffering present mode if available
    for (VkPresentModeKHR availableMode : presentModes) {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availableMode;
        }
    }

    // Use FIFO mode as our fallback, since it's the only guaranteed mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR Swapchain::pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
    for (VkSurfaceFormatKHR availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // As a fallback choose the first format
    // TODO Could establish a ranking and pick best one
    return formats[0];
}

Swapchain::SwapchainConfiguration Swapchain::querySwapchainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    SwapchainConfiguration config = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &config.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount > 0) {
        config.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                             surface,
                                             &formatCount,
                                             config.surfaceFormats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount > 0) {
        config.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                                  surface,
                                                  &presentModeCount,
                                                  config.presentModes.data());
    }

    return config;
}