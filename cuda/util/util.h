#pragma once

#include <optix.h>

#include "vec_math.h"
#include "device_types.h"

static __forceinline__ __device__ bool isOccluded(
    OptixTraversableHandle handle,
    float3                 ray_origin,
    float3                 ray_direction,
    float                  tmin,
    float                  tmax
)
{
    optixTraverse(
        handle,
        ray_origin,
        ray_direction,
        tmin,
        tmax, 0.0f,
        OptixVisibilityMask(1),
        OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_ANYHIT,
        0,
        1,
        0
    );
    return optixHitObjectIsHit();
}


static __forceinline__ __device__ void generateCameraRay(
    const CameraData& cam,
    float x,
    float y,
    unsigned int width,
    unsigned int height,
    float2 u,
    float3& rayOrigin,
    float3& rayDirection)
{
    const float3 horizontal = cam.viewportWidth * cam.u;
    const float3 vertical = cam.viewportHeight * cam.v;

    const float3 deltaU = horizontal / static_cast<float>(width);
    const float3 deltaV = vertical / static_cast<float>(height);

    const float3 upperLeftPixel =
        cam.origin
        - 0.5f * (horizontal - vertical)
        - cam.focusDist * cam.w;

    const float3 pixelPos =
        upperLeftPixel
        + x * deltaU
        - y * deltaV;

    const float2 unitPos = sampling::sample_uniform_disk(u);
    const float3 lensPos = cam.origin +
        (cam.u * unitPos.x + cam.v * unitPos.y) * cam.lensRadius;

    rayOrigin = lensPos;
    rayDirection = normalize(pixelPos - lensPos);
}


__forceinline__ __device__ float2 directionToUV(const float3& dir) {
    float3 d = normalize(dir);

    const float phi = std::atan2(d.z, d.x);
    const float theta = std::acos(clamp(d.y, -1.0f, 1.0f));

    const float u = (phi + M_PIf) / (2.0f * M_PIf);
    const float v = theta / M_PIf;

    return make_float2(u, v);
}