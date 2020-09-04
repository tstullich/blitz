#include "gltfloader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

tinygltf::Model GLTFLoader::load(const std::string &filePath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string error;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&model, &error, &warn, filePath)) {
        std::cout << error << std::endl;
        throw std::runtime_error("Unable to parse GLTF!");
    }

    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }

    return model;
}

