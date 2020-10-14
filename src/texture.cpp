#include "texture.h"

Texture::Texture(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const tinygltf::Image &image) {
    allocateMemory(physicalDevice, logicalDevice, image);
}

void Texture::allocateMemory(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const tinygltf::Image &image) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(image.width);
    imageInfo.extent.height = static_cast<uint32_t>(image.height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = pickMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate image memory!");
    }

    vkBindImageMemory(logicalDevice, textureImage, textureMemory, 0);
}

void Texture::cleanup(VkDevice logicalDevice) {
    vkDestroyImage(logicalDevice, textureImage, nullptr);
    vkFreeMemory(logicalDevice, textureMemory, nullptr);
}

uint32_t Texture::pickMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1u << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Unable to find proper memory type on the GPU!");
}