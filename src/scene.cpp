#include "scene.h"

blitz::Scene::Scene(const std::string &filePath) {
    loadScene(filePath);
}

//void blitz::Scene::loadCamera(tinygltf::Model &model, tinygltf::Node &node) {
//    // TODO Figure out why this does not work!
//    auto translation = glm::mat4(1.0f);
//    if (!node.translation.empty()) {
//        auto translationVec = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
//        translation = glm::translate(translation, translationVec);
//    }
//
//    auto rotation = glm::mat4(1.0f);
//    if (!node.rotation.empty()) {
//        auto rotationQuat = glm::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
//        rotation = glm::mat4_cast(rotationQuat);
//    }
//
//    auto scale = glm::mat4(1.0f);
//    if (!node.scale.empty()) {
//        // Unlikely that the camera node will have a scale vector
//        // but keeping it just in case
//        auto scalingVec = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
//        scale = glm::scale(scale, scalingVec);
//    }
//
//    cam.model = translation * rotation * scale;
//}

void blitz::Scene::loadNode(tinygltf::Model &model, tinygltf::Node &node) {
    if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
        loadMesh(model, model.meshes[node.mesh]);
    }

    // TODO Check if this is the best way to setup the camera
    //if (node.name == "Camera") {
    //    loadCamera(model, node);
    //}

    // Parse child nodes
    for (int i : node.children) {
        loadNode(model, model.nodes[i]);
    }
}

void blitz::Scene::loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
    for (auto primitive : mesh.primitives) {
        if (primitive.indices >= 0) {
            // Mesh supports indexing. Load the indices
            loadMeshIndices(model, primitive);
        }

        loadVertexAttributes(model, mesh, primitive);

        if (primitive.material >= 0) {
            loadMeshMaterial(model, primitive);
        }
    }
}

void blitz::Scene::loadMeshIndices(tinygltf::Model &model, tinygltf::Primitive &primitive) {
    auto indicesAccessor = model.accessors[primitive.indices];
    auto bufferView = model.bufferViews[indicesAccessor.bufferView];
    auto buffer = model.buffers[bufferView.buffer];
    auto bufferOffset = bufferView.byteOffset;
    auto offset = indicesAccessor.byteOffset / sizeof(float);
    if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        auto indicesBytes = reinterpret_cast<unsigned short*>(buffer.data.data() + bufferOffset);
        auto bufferStart = indicesBytes + offset;
        auto bufferEnd = indicesBytes + bufferView.byteLength / sizeof(unsigned short);
        std::vector<uint32_t> indices(bufferStart, bufferEnd);
        vertIndices.insert(vertIndices.end(), indices.begin(), indices.end());
    } else {
        auto indicesBytes = reinterpret_cast<unsigned int*>(buffer.data.data() + bufferOffset);
        auto bufferStart = indicesBytes + offset;
        auto bufferEnd = indicesBytes + bufferView.byteLength / sizeof(unsigned int);
        std::vector<uint32_t> indices(bufferStart, bufferEnd);
        vertIndices.insert(vertIndices.end(), indices.begin(), indices.end());
    }
}

void blitz::Scene::loadMeshMaterial(tinygltf::Model &model, tinygltf::Primitive &primitive) {
    // TODO Expand the logic here later to properly handle PBR-related things
    auto material = model.materials[primitive.material];
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        auto tex = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
        textureImage = model.images[tex.source];
        sampler = model.samplers[tex.sampler];
    }
}

void blitz::Scene::loadScene(const std::string &filePath) {
    auto model = GLTFLoader::load(filePath);
    auto scene = model.scenes[model.defaultScene];
    for (int nodeId : scene.nodes) {
        loadNode(model, model.nodes[nodeId]);
    }
}

void blitz::Scene::loadVertexAttributes(tinygltf::Model &model, tinygltf::Mesh &mesh, tinygltf::Primitive &primitive) {
    auto positionIndex = primitive.attributes.find("POSITION")->second;
    auto normalIndex = primitive.attributes.find("NORMAL")->second;
    auto texIndex = primitive.attributes.find("TEXCOORD_0")->second;

    if (positionIndex >= 0) {
        auto positionAccessor = model.accessors[positionIndex];
        auto bufferView = model.bufferViews[positionAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];
        // Kinda hacky way of initializing the vertices buffer since we are assuming
        // the POSITION attribute is always present (although that seems quite reasonable).
        vertices.resize(positionAccessor.count);

        // Calculate the necessary offsets for the position of the vertices out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = positionAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> positions(bufferStart, bufferEnd);
        for (size_t i = 0; i < positionAccessor.count; ++i) {
            vertices[i].position[0] = positions[i * 3];
            vertices[i].position[1] = positions[i * 3 + 1];
            vertices[i].position[2] = positions[i * 3 + 2];
        }
    }

    if (normalIndex >= 0) {
        auto normalAccessor = model.accessors[normalIndex];
        auto bufferView = model.bufferViews[normalAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];

        // Calculate the necessary offsets for the vertex normals out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = normalAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> normals(bufferStart, bufferEnd);
        for (size_t i = 0; i < normalAccessor.count; ++i) {
            vertices[i].normal[0] = normals[i * 3];
            vertices[i].normal[1] = normals[i * 3 + 1];
            vertices[i].normal[2] = normals[i * 3 + 2];
        }
    }

    if (texIndex >= 0) {
        auto texAccessor = model.accessors[texIndex];
        auto bufferView = model.bufferViews[texAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];

        // Calculate the necessary offsets for the vertex normals out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = texAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> texCoords(bufferStart, bufferEnd);
        for (size_t i = 0; i < texAccessor.count; ++i) {
            vertices[i].texCoord[0] = texCoords[i * 2];
            vertices[i].texCoord[1] = texCoords[i * 2 + 1];
        }
    }
}

