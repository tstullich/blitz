#pragma once

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex {
    // Just have the position + color for now. Will expand to more attributes later
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription description = {};
        // TODO Make the binding location a bit more dynamic
        description.binding = 0;
        description.stride = sizeof(Vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return description;
    };

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> descriptions = {};

        descriptions[0].binding = 0; // Needs to match info from VkVertexInputBindingDescription
        descriptions[0].location = 0;
        descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[0].offset = offsetof(Vertex, position);

        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[1].offset = offsetof(Vertex, color);

        descriptions[2].binding = 0;
        descriptions[2].location = 2;
        descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[2].offset = offsetof(Vertex, normal);

        return descriptions;
    }
};