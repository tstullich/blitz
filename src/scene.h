#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "camera.h"
#include "gltfloader.h"
#include "image.h"
#include "light.h"
#include "vertex.h"

namespace blitz {
struct Scene {
    Scene() = default;

    // TODO Remove the aspectRatio parameter here
    explicit Scene(const std::string &filePath, float aspectRatio);

    void loadCamera(tinygltf::Model &model, tinygltf::Camera &cam, glm::mat4 &transform);

    void loadLight(tinygltf::Model &model, glm::mat4 &transform);

    void loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh, glm::mat4 &transform);

    void loadMeshMaterial(tinygltf::Model &model, tinygltf::Primitive &primitive);

    void loadMeshIndices(tinygltf::Model &model, tinygltf::Primitive &primitive);

    void loadNode(tinygltf::Model &mode, tinygltf::Node &node, glm::mat4 &transform);

    void loadScene(const std::string &filePath, float aspectRatio);

    static glm::mat4 loadTransform(const tinygltf::Node &node);

    void loadVertexAttributes(tinygltf::Model &model, tinygltf::Mesh &mesh, tinygltf::Primitive &primitive);

    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> vertIndices = {};

    tinygltf::Image textureImage;

    // TODO Figure out a better way to store these values
    Camera camera;
    float camYFov = glm::radians(60.0f);
    float camAspectRatio;
    float camZNear = 0.1f;
    float camZFar = 100.0f;

    Light light;
};
} // namespace blitz