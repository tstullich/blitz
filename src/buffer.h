#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan.h>

#include "vertex.h"

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

    inline VkBuffer getBuffer() const { return buffer; }

protected:
    static void copyBuffer(const BufferContext &ctx, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    static void init(const BufferContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
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