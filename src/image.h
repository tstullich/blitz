#pragma once

#include <stdexcept>

#include <vulkan/vulkan.h>

class Image {
public:
    Image() = default;

    Image(VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo);

    void bindImage(VkDevice logicalDevice, uint32_t memoryTypeIndex);

    void cleanup(VkDevice logicalDevice);

    inline VkFormat getFormat() const { return format; }

    inline VkImage getImage() const { return image; }

    inline VkImageView getView() const { return imageView; }

private:
    void createImage(VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo);

    void createImageView(VkDevice logicalDevice);

    VkImage image = {};
    VkImageView imageView = {};
    VkFormat format = {};
    VkDeviceMemory imageMemory = {};
};