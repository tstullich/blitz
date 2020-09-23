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

    auto extension = tinygltf::GetFilePathExtension(filePath);
    bool loaded;
    if (extension == "glb") {
        loaded = loader.LoadBinaryFromFile(&model, &error, &warn, filePath);
    } else {
        loaded = loader.LoadASCIIFromFile(&model, &error, &warn, filePath);
    }

    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }

    if (!loaded) {
        std::cout << "Error: " << error << std::endl;
        throw std::runtime_error("Unable to load glTF file!");
    }

    return model;
}

