#pragma once

#include <vulkan/vulkan.h>

#include "tiny_gltf.h"

#include "buffer.h"

class Texture {
public:
    Texture() = default;

    Texture(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const tinygltf::Image &image);

    void cleanup(VkDevice logicalDevice);

    inline VkImage getImage() const { return textureImage; }

private:
    VkImage textureImage = {};
    VkDeviceMemory textureMemory = {};

    void allocateMemory(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const tinygltf::Image &image);

    uint32_t pickMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
