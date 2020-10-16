#include "texture.h"

Texture::Texture(VkDevice logicalDevice, const tinygltf::Image &image) {
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

    textureImage = Image(logicalDevice, imageInfo);
}

void Texture::bind(VkDevice logicalDevice, uint32_t memoryTypeIndex) {
    textureImage.bindImage(logicalDevice, memoryTypeIndex, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::cleanup(VkDevice logicalDevice) {
    textureImage.cleanup(logicalDevice);
}