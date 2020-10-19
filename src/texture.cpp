#include "texture.h"

Texture::Texture(VkDevice logicalDevice, const tinygltf::Image &image, bool createMips) {
    auto imageWidth = static_cast<uint32_t>(image.width);
    auto imageHeight = static_cast<uint32_t>(image.height);
    auto mipLevels = createMips ? calculateMipLevels(imageWidth, imageHeight) : 1;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = imageWidth;
    imageInfo.extent.height = imageHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (createMips) {
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT;
    } else {
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    textureImage = Image(logicalDevice, imageInfo);
}

void Texture::bind(VkDevice logicalDevice, uint32_t memoryTypeIndex) {
    textureImage.bindImage(logicalDevice, memoryTypeIndex, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::cleanup(VkDevice logicalDevice) {
    textureImage.cleanup(logicalDevice);
}

uint32_t Texture::calculateMipLevels(const uint32_t &texWidth, const uint32_t &texHeight) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
}