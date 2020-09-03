#include "buffer.h"

// TODO Create a separate command pool for copying buffers and use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
// during command pool creation
void Buffer::copyBuffer(const BufferContext &ctx, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = ctx.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(ctx.logicalDevice, &allocateInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create memory transfer command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Unable to record one-time command buffer!");
    }

    // Record the copy command to our one-time buffer
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Need this for GLTF later
    copyRegion.dstOffset = 0; // Need this for GLTF later
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    // Don't need to set semaphores since we do not need to sync
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // TODO If we want to upload more vertex buffers later we could use some fences here
    vkQueueSubmit(ctx.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.queue);
    vkFreeCommandBuffers(ctx.logicalDevice, ctx.commandPool, 1, &commandBuffer);
}


void Buffer::cleanup(VkDevice logicalDevice) {
    vkDestroyBuffer(logicalDevice, buffer, nullptr);
    vkFreeMemory(logicalDevice, deviceMemory, nullptr);
}

void Buffer::init(const BufferContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buf, VkDeviceMemory &devMem) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.logicalDevice, &bufferInfo, nullptr, &buf) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.logicalDevice, buf, &memRequirements);

    // Allocate memory on the GPU. The type of memory also needs to be visible for the host
    // so we search for HOST_VISIBLE and HOST_COHERENT memory types
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = pickMemoryType(ctx.physicalDevice, memRequirements.memoryTypeBits, properties);
    if (vkAllocateMemory(ctx.logicalDevice, &allocateInfo, nullptr, &devMem) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate device memory!");
    }

    // Bind the allocated memory to the vertex buffer
    vkBindBufferMemory(ctx.logicalDevice, buf, devMem, 0);
}

uint32_t Buffer::pickMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1u << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Unable to find proper memory type on the GPU!");
}


void VertexBuffer::create(const BufferContext &ctx, const std::vector<Vertex> &vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // We use a staging buffer to transfer the host buffer data to the GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    init(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

    void *data;
    vkMapMemory(ctx.logicalDevice, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(ctx.logicalDevice, stagingMemory);

    init(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
         VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, buffer, deviceMemory);

    // Copy the data into GPU memory
    copyBuffer(ctx, stagingBuffer, buffer, bufferSize);

    // Cleanup staging buffers
    vkDestroyBuffer(ctx.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(ctx.logicalDevice, stagingMemory, nullptr);
}

void IndexBuffer::create(const BufferContext &ctx, const std::vector<uint32_t> &vertIndices) {
    VkDeviceSize bufferSize = sizeof(vertIndices[0]) * vertIndices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    init(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

    void *data;
    vkMapMemory(ctx.logicalDevice, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertIndices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(ctx.logicalDevice, stagingMemory);

    init(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);

    copyBuffer(ctx, stagingBuffer, buffer, bufferSize);

    // Cleanup staging buffers
    vkDestroyBuffer(ctx.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(ctx.logicalDevice, stagingMemory, nullptr);
}

