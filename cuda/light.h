#pragma once

#include "device_types.h"
#include "util/sampling.h"

#include <stdio.h>

struct LightSample {
    float3 pos{ 0.0f };
    float pdf = 1.0f;
    float3 emission{ 0.0f };
    float3 normal{ 0.0f };
    bool isHittable = true;
};

__forceinline__ __device__ LightSample sample_triangle_light(
    const Light& light,
    const Material& mat,
    const float3& hitPos,
    const float2& u) {

    LightSample s{};

    s.normal = light.geoLight.triangle.normalFlat;
    s.emission = mat.emission;

    s.pos = sampling::sample_triangle(
        light.geoLight.triangle.v0,
        light.geoLight.triangle.v1,
        light.geoLight.triangle.v2,
        u
    );

    // pdf in solid angle space
    const float3 e1 = light.geoLight.triangle.v1 - light.geoLight.triangle.v0;
    const float3 e2 = light.geoLight.triangle.v2 - light.geoLight.triangle.v0;
    s.pdf = sampling::pdf_uniform_triangle(e1, e2, s.normal, hitPos, s.pos);

    s.isHittable = true;

    return s;

}

__forceinline__ __device__ LightSample sample_area_light(
    const Light& light,
    const float3& hitPos,
    const float2& u) {

    LightSample s{};

    const float3 uL = light.absLight.u;
    const float3 vL = light.absLight.v;

    s.normal = normalize(cross(uL, vL));
    s.emission = light.absLight.emission;
    s.pos = light.absLight.pos + u.x * uL + u.y * vL;

    // pdf in solid angle space
    float3 wi = s.pos - hitPos;
    const float d2 = dot(wi, wi);
    wi = normalize(wi);
    const float cosThetaJ = max(1e-8f, dot(s.normal, -wi));
    s.pdf = (1.0f / length(cross(uL, vL))) * d2 / cosThetaJ;

    s.isHittable = false;

    return s;

}

__forceinline__ __device__ LightSample sample_lights(
    Light* lights, const size_t numLights, 
    Material* materials, 
    const float3& hitPos, 
    const float3& u) {

    LightSample s{};

    if (numLights == 0) 
        return s;

    // randomly choose one light source
    const unsigned int lightIdx = static_cast<unsigned int>(u.z * numLights);
    const Light& light = lights[lightIdx];

    // sample point on light according to light type
    LightSample temp;
    switch (light.type) {
    case LightType::TRIANGLE_LIGHT:
        const Material& mat = materials[light.geoLight.materialIndex];
        temp = sample_triangle_light(light, mat, hitPos, make_float2(u));
        break;
    case LightType::AREA_LIGHT:
        temp = sample_area_light(light, hitPos, make_float2(u));
        break;
    default:
        return s;
    }

    s.normal = temp.normal;
    s.emission = temp.emission;
    s.pos = temp.pos;
    s.pdf = temp.pdf / numLights;
    s.isHittable = temp.isHittable;

    return s;
}