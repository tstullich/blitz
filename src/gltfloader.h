#pragma once

#include <iostream>
#include <string>

#include "tiny_gltf.h"

namespace blitz {
class GLTFLoader {
public:
    GLTFLoader() = delete;

    static tinygltf::Model load(const std::string &filePath);
};
} // namespace blitz