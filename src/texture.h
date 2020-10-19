#pragma once

#include <vulkan/vulkan.h>

#include "tiny_gltf.h"

#include "buffer.h"
#include "image.h"

class Texture {
public:
    Texture() = default;

    Texture(VkDevice logicalDevice, const tinygltf::Image &image, bool createMips);

    void bind(VkDevice logicalDevice, uint32_t memoryTypeIndex);

    void cleanup(VkDevice logicalDevice);

    inline uint32_t getHeight() const { return textureImage.getHeight(); }

    inline VkImageView getImageView() const { return textureImage.getView(); }

    inline uint32_t getMipLevels() const { return textureImage.getMipLevels(); }

    inline Image getTexImage() const { return textureImage; }

    inline uint32_t getWidth() const { return textureImage.getWidth(); }

private:
    static uint32_t calculateMipLevels(const uint32_t &texWidth, const uint32_t &texHeight);

    Image textureImage = {};
};
