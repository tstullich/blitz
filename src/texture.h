#pragma once

#include <vulkan/vulkan.h>

#include "tiny_gltf.h"

#include "buffer.h"
#include "image.h"

class Texture {
public:
    Texture() = default;

    Texture(VkDevice logicalDevice, const tinygltf::Image &image);

    void bind(VkDevice logicalDevice, uint32_t memoryTypeIndex);

    void cleanup(VkDevice logicalDevice);

    inline VkImage getImage() const { return textureImage.getImage(); }

    inline VkImageView getImageView() const { return textureImage.getView(); }

private:
    Image textureImage = {};
};
