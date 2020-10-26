#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace blitz {
struct Camera {
    Camera() = default;

    explicit Camera(float aspectRatio) {
        model = glm::mat4(1.0f); // Center model where it is in the scene
        view = glm::lookAt(glm::vec3(1.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        proj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);
        proj[1][1] *= -1;
    }

    void setModelMatrix(const glm::mat4 &modelMatrix) {
        model = modelMatrix;
    }

    void setViewMatrix (const glm::vec3 &position, const glm::vec3 &lookAt) {
        view = glm::lookAt(position, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void setProjectionMatrix(float fovY, float aspectRatio, float zNear, float zFar) {
        auto projection = glm::perspective(fovY, aspectRatio, zNear, zFar);
        projection[1][1] *= -1; // Flip the y-axis for Vulkan compatibility
        proj = projection;
    }

    glm::mat4 model = {};
    glm::mat4 view = {};
    glm::mat4 proj = {};
};
} // namespace blitz