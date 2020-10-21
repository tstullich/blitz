#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace blitz {
struct Camera {
    Camera() = default;

    explicit Camera(float aspectRatio) {
        model = glm::mat4(1.0f);
        view = glm::lookAt(glm::vec3(1.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        proj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 10.0f);
        proj[1][1] *= -1;
    }

    explicit Camera(const glm::vec3 &eye, const glm::vec3 &at, float aspectRatio) {
        model = glm::mat4(1.0f);
        view = glm::lookAt(eye, at, glm::vec3(0.0f, 0.0f, 1.0f));
        proj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 10.0f);
        proj[1][1] *= -1;
    }

    glm::mat4 model = {};
    glm::mat4 view = {};
    glm::mat4 proj = {};
};
} // namespace blitz