#pragma once

#include <glm/glm.hpp>

namespace blitz {
struct Light {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 emissiveColor;
};
}