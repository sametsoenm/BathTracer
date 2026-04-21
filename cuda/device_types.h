#pragma once

#include <optix.h>
#include <vector_types.h>
#include <texture_types.h>
#include "rendersettings.h"

#include <string_view>
#include <string>

struct RadiancePRD {
    float3 attenuation;
    unsigned int seed;
    unsigned int depth;

    float3 prevHitPos;
    float3 radiance;
    float3 rayO;
    float3 rayDir;
    unsigned int exit = 0;
    unsigned int prevDelta;
    float prevScatterPdf;
};

struct CameraData {
    float3 origin;

    float3 u;
    float3 v;
    float3 w;

    float viewportWidth;
    float viewportHeight;
    float focusDist;
    float lensRadius;
};


enum class MaterialType {
    LAMBERT_DIFFUSE,
    MIRROR,
    SPECULAR_MICROFACET,
    SMOOTH_DIELECTRIC,
    ROUGH_DIELECTRIC,
    EMISSIVE_DIFFUSE
};

struct Material {
    MaterialType type;
    float3 color;  // albedo for diffuse, F0 for metals
    uint32_t colorTexIdx;
    float3 emission;
    float3 reflectance;
    float alpha;
    uint32_t alphaTexIdx;
    float eta;
    bool isDelta = false;
    bool hasNormalMap = false;
};

struct TriangleData {
    float3 v0;
    float3 v1;
    float3 v2;
    float3 normalFlat;
    bool hasVertexNormals = false;
    float3 n0;
    float3 n1;
    float3 n2;
    float2 uv0;
    float2 uv1;
    float2 uv2;
};

struct GeometryData {
    TriangleData triangle;
    uint32_t materialIndex;
};

enum LightType {
    TRIANGLE_LIGHT, // geometric
    AREA_LIGHT, // abstract
    POINT_LIGHT, // abstract
    DIRECTIONAL_LIGHT // abstract
};

struct AbstractLight {
    float3 pos;
    float3 dir;
    float3 emission;
    float3 u, v;
};

struct Light {
    LightType type;
    union {
        GeometryData geoLight;
        AbstractLight absLight;
    };
};

enum class FogMode {
    SINGLE_SCATTER,
    MULTIPLE_SCATTER
    // after adding new type also adjust the corresp. array!!!
};

constexpr std::string_view to_string(FogMode m) {
    switch (m) {
    case FogMode::SINGLE_SCATTER: return "Single Scattering";
    case FogMode::MULTIPLE_SCATTER: return "Multiple Scattering";
    default: return "Unkown";
    }
}

struct HitData {
    float t = settings::T_MAX;
    float3 pos{ 0.0f };
    float2 uv{ 0.0f };

    uint32_t triangle_id;
    float3 geo_normal{ 0.0f };
    float3 shading_normal{ 0.0f };
    float3 tangent{ 0.0f };

    bool front_face = true;
};

struct LaunchParams {
    uchar4* image;
    float4* accumBuffer;
    unsigned int width;
    unsigned int height;
    unsigned int subFrameIdx;
    unsigned int samplesPerFrame;
    unsigned int maxDepth;

    OptixTraversableHandle traversable;

    CameraData camera;
    GeometryData* geometry;
    Light* lights;
    size_t numLights;
    Material* materials;
    cudaTextureObject_t* textures;
    cudaTextureObject_t envmap;
    bool envmapEnabled;

    bool fogEnabled;
    float fogDensity;
    FogMode fogMode;
};

extern "C" __constant__ LaunchParams params;

struct EmptyData {};
struct MissData
{
};

struct HitGroupData
{
};