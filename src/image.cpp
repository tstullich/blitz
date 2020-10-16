#include "image.h"

Image::Image(VkDevice logicalDevice, const VkImageCreateInfo &imageInfo) {
    createImage(logicalDevice, imageInfo);
}

void Image::bindImage(VkDevice logicalDevice, uint32_t memoryTypeIndex, VkImageAspectFlagBits aspectFlags) {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate image memory!");
    }

    vkBindImageMemory(logicalDevice, image, imageMemory, 0);

    // Create an image view after the image has been bound to GPU memory
    createImageView(logicalDevice, aspectFlags);
}

void Image::cleanup(VkDevice logicalDevice) {
    vkDestroyImageView(logicalDevice, imageView, nullptr);
    vkDestroyImage(logicalDevice, image, nullptr);
    vkFreeMemory(logicalDevice, imageMemory, nullptr);
}

void Image::createImage(VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo) {
    // Store the image format for later use
    format = imageCreateInfo.format;

    if (vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create image!");
    }
}

void Image::createImageView(VkDevice logicalDevice, VkImageAspectFlagBits aspectFlags) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create image view!");
    }
}