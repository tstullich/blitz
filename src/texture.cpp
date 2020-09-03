#include "texture.h"

Texture::Texture(const TextureContext &ctx) {
    // Check if format is supported by system
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(ctx.physicalDevice, ctx.texFormat, &properties);

    if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
        throw std::runtime_error("Requested image format does not support storage operations!");
    }

    
}