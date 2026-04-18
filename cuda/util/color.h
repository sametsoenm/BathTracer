#pragma once

#include "vec_math.h"

namespace color {

    __forceinline__ __device__ float linear_to_srgb(float x) {
        x = clamp(x, 0.0f, 1.0f);
        if (x <= 0.0031308f)
            return 12.92f * x;
        return 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
    }

    __forceinline__ __device__ float3 linear_to_srgb(const float3& x) {
        return make_float3(
            linear_to_srgb(x.x),
            linear_to_srgb(x.y),
            linear_to_srgb(x.z)
        );
    }

    __forceinline__ __device__ uchar4 make_color(const float3& x) {
        float3 srgb = linear_to_srgb(clamp(x, 0.0f, 1.0f));
        return make_uchar4(
            static_cast<unsigned char>(255.0f * srgb.x),
            static_cast<unsigned char>(255.0f * srgb.y),
            static_cast<unsigned char>(255.0f * srgb.z),
            255
        );
    }

}