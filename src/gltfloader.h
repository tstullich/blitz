#pragma once

#include <iostream>
#include <string>

#include "cgltf.h"

class GLTFLoader {
public:
    GLTFLoader() = delete;

    static cgltf_data * load(const std::string &filePath);

    static void destroy(cgltf_data *data);
};
