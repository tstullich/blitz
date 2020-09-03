#include <stdexcept>

#include <vulkan/vulkan.h>

class Texture {
public:
    struct TextureContext {
        VkPhysicalDevice physicalDevice;
        VkFormat texFormat;
        uint32_t width;
        uint32_t height;
    };

    Texture(const TextureContext &ctx);

    inline uint32_t getHeight() const { return height; }

    inline uint32_t getWidth() const { return width; }

private:
    VkImage image;
    VkImageView imageView;

    uint32_t width;
    uint32_t height;
};
