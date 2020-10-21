#pragma once

#include <stdexcept>

#include <vulkan/vulkan.h>

namespace blitz {
class Image {
public:
    Image() = default;

    Image(VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo);

    void bindImage(VkDevice logicalDevice, uint32_t memoryTypeIndex, VkImageAspectFlagBits aspectFlags);

    void cleanup(VkDevice logicalDevice);

    inline VkFormat getFormat() const { return format; }

    inline uint32_t getHeight() const { return extent.height; }

    inline VkImage getImage() const { return image; }

    inline uint32_t getMipLevels() const { return mipLevels; }

    inline VkImageView getView() const { return imageView; }

    inline uint32_t getWidth() const { return extent.width; }

private:
    void createImage(VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo);

    void createImageView(VkDevice logicalDevice, VkImageAspectFlagBits aspectFlags);

    VkImage image = {};
    VkImageView imageView = {};
    VkFormat format = {};
    VkDeviceMemory imageMemory = {};
    VkExtent3D extent = {};
    uint32_t mipLevels = 1;
};
} // namespace blitz