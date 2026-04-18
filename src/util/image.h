#pragma once

#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace image {

    inline float linear_to_srgb(float x) {
        x = glm::clamp(x, 0.0f, 1.0f);
        if (x <= 0.0031308f)
            return 12.92f * x;
        return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
    }

    inline float srgb_to_linear(float x) {
        if (x <= 0.04045f)
            return x / 12.92f;
        return std::pow((x + 0.055f) / 1.055f, 2.4f);
    }

}