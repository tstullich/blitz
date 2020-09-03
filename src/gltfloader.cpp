#include "gltfloader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

cgltf_data* GLTFLoader::load(const std::string &filePath) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filePath.c_str(), &data);
    if (result != cgltf_result_success) {
        throw std::runtime_error("Unable to open GLTF file!");
    }
    return data;
}

void GLTFLoader::destroy(cgltf_data *data) {
    cgltf_free(data);
}
