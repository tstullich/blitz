#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gltfloader.h"
#include "image.h"
#include "vertex.h"

namespace blitz {
struct Scene {
    Scene() = default;

    explicit Scene(const std::string &filePath);

    //void loadCamera(tinygltf::Model &model, tinygltf::Node &node);

    void loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh);

    void loadMeshMaterial(tinygltf::Model &model, tinygltf::Primitive &primitive);

    void loadMeshIndices(tinygltf::Model &model, tinygltf::Primitive &primitive);

    void loadNode(tinygltf::Model &mode, tinygltf::Node &node);

    void loadScene(const std::string &filePath);

    void loadVertexAttributes(tinygltf::Model &model, tinygltf::Mesh &mesh, tinygltf::Primitive &primitive);

    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> vertIndices = {};
    tinygltf::Image textureImage;
    tinygltf::Sampler sampler;
};
} // namespace blitz