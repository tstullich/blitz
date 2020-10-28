#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace blitz {
struct Camera {
    Camera() = default;

    explicit Camera(float aspectRatio) {
        glm::vec3 position = glm::vec3(1.0f);
        model = glm::mat4(1.0f); // Center model where it is in the scene
        view = glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        proj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);
        proj[1][1] *= -1;
    }

    void setModelMatrix(const glm::mat4 &modelMatrix) {
        model = modelMatrix;
    }

    void setViewMatrix (const glm::vec3 &eye, const glm::vec3 &lookAt) {
        glm::vec3 position = eye;
        view = glm::lookAt(position, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void setProjectionMatrix(float fovY, float aspectRatio, float zNear, float zFar) {
        auto projection = glm::perspective(fovY, aspectRatio, zNear, zFar);
        projection[1][1] *= -1; // Flip the y-axis for Vulkan compatibility
        proj = projection;
    }

    alignas(16) glm::mat4 model = {};
    alignas(16) glm::mat4 view = {};
    alignas(16) glm::mat4 proj = {};
    alignas(16) glm::vec3 position = {};
};
} // namespace blitz