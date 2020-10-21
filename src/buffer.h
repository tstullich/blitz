#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan.h>

#include "tiny_gltf.h"

#include "camera.h"
#include "vertex.h"

// TODO Templatize this logic since vertex and index buffer creation is almost identical
namespace blitz {
class Buffer {
public:
    struct BufferContext {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkQueue queue;
        VkCommandPool commandPool;
    };

    Buffer() = default;

    void cleanup(VkDevice logicalDevice);

    inline VkDeviceMemory getDeviceMemory() const { return deviceMemory; }

    inline VkBuffer getBuffer() const { return buffer; }

protected:
    static void copyBuffer(const BufferContext &ctx, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    static void allocateMemory(const BufferContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkBuffer &buf, VkDeviceMemory &devMem);

    static uint32_t pickMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties);

    VkBuffer buffer;
    VkDeviceMemory deviceMemory;
};

class VertexBuffer : public Buffer {
public:
    VertexBuffer() = default;

    void create(const BufferContext &ctx, const std::vector<Vertex> &vertices);
};

class IndexBuffer : public Buffer {
public:
    IndexBuffer() = default;

    void create(const BufferContext &ctx, const std::vector<uint32_t> &vertIndices);
};

class UniformBuffer : public Buffer {
public:
    UniformBuffer() = default;

    void create(const BufferContext &ctx, const Camera &camera);
};

class TextureBuffer : public Buffer {
public:
    TextureBuffer() = default;

    void create(const BufferContext &ctx, const tinygltf::Image &textureImage);
};
} // namespace blitz