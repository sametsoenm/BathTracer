#include "materials/material.h"
#include "volume.h"
#include "util/color.h"
#include "shading.h"

#include <stdio.h>

extern "C" {
	__constant__ LaunchParams params;
}

extern "C" __global__ void __raygen__rg()
{
	const uint3 idx = optixGetLaunchIndex();
    unsigned int seed = tea<4>(idx.y * params.width + idx.x, params.subFrameIdx);
    const unsigned int maxDepth = params.maxDepth;
    const unsigned int spp = params.samplesPerFrame;

    float3 result = make_float3(0.0f);

    for (unsigned int s = 0; s < spp; s++) {

        const float x_jit = static_cast<float>(idx.x) + rnd(seed);
        const float y_jit = static_cast<float>(idx.y) + rnd(seed);

        float3 origin;
        float3 direction;
        generateCameraRay(params.camera,
            x_jit, y_jit,
            params.width, params.height,
            rnd2(seed),
            origin, direction);

        float3 throughput = make_float3(1.0f);
        RadiancePRD prd = {};
        prd.prevDelta = 0u;
        prd.seed = seed;

        for (unsigned int depth = 0; depth < maxDepth; depth++) {
            prd.depth = depth;

            unsigned int p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19;
            p3 = prd.seed;
            p4 = prd.depth;
            p5 = __float_as_uint(prd.prevHitPos.x);
            p6 = __float_as_uint(prd.prevHitPos.y);
            p7 = __float_as_uint(prd.prevHitPos.z);
            p18 = prd.prevDelta;
            p19 = __float_as_uint(prd.prevScatterPdf);

            optixTraverse(
                params.traversable,
                origin,
                direction,
                settings::T_MIN,    // tmin
                settings::T_MAX,    // tmax
                0.0f,               // rayTime
                OptixVisibilityMask(255),
                OPTIX_RAY_FLAG_DISABLE_ANYHIT,
                0,                  // SBT offset
                1,                  // SBT stride
                0,                  // missSBTIndex
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19 
            );

            const float t_surface = optixHitObjectIsHit() 
                ? optixHitObjectGetRayTmax() 
                : settings::T_MAX;
            const float t_medium = params.fogEnabled 
                ? -logf(rnd(prd.seed)) / params.fogDensity 
                : settings::T_MAX;

            if (t_medium < t_surface) {
                prd.rayO = origin;
                prd.rayDir = direction;
                volumeEvent(params.fogMode, prd, t_medium); 
            }
            else {
                p3 = prd.seed;
                optixInvoke(
                    p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19
                );
                prd.attenuation = make_float3(__uint_as_float(p0), __uint_as_float(p1), __uint_as_float(p2));
                prd.seed = p3;
                prd.prevHitPos = make_float3(__uint_as_float(p5), __uint_as_float(p6), __uint_as_float(p7));
                prd.radiance = make_float3(__uint_as_float(p8), __uint_as_float(p9), __uint_as_float(p10));
                prd.rayO = make_float3(__uint_as_float(p11), __uint_as_float(p12), __uint_as_float(p13));
                prd.rayDir = make_float3(__uint_as_float(p14), __uint_as_float(p15), __uint_as_float(p16));
                prd.exit = p17;
                prd.prevDelta = p18;
                prd.prevScatterPdf = __uint_as_float(p19);
            }

            result += prd.radiance * throughput;

            if (prd.exit)
                break;

            throughput *= prd.attenuation;

            if (depth >= 3) {
                // calc luminance
                float p = 0.2126f * throughput.x
                    + 0.7152f * throughput.y
                    + 0.0722f * throughput.z;
                p = clamp(p, 0.05f, 0.95f);

                if (rnd(prd.seed) > p)
                    break;

                throughput /= p;
            }

            origin = prd.rayO;
            direction = prd.rayDir;

        }
        // seed = prd.seed; // necessary?
    }

    result /= spp;

    const unsigned int imageIdx = idx.y * params.width + idx.x;
    params.accumBuffer[imageIdx] += make_float4(result, 1.0f);
    const float3 color = 
        make_float3(params.accumBuffer[imageIdx]) 
        / static_cast<float>(params.subFrameIdx + 1u);
    params.image[imageIdx] = color::make_color(color);

}

extern "C" __global__ void __miss__ms()
{
    unsigned int depth = optixGetPayload_4();

    float4 tex;
    if (depth == 0 || !params.envmapEnabled) {
        tex = make_float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else {
        const float3 dir = optixGetWorldRayDirection();
        const float2 uv = directionToUV(dir);
        tex = tex2D<float4>(params.envmap, uv.x, uv.y);

        // temporarly clamp to reduce fireflies
        tex = clamp(tex, 0.0f, 10.0f); 
    }

    // store radiance
    optixSetPayload_8(__float_as_uint(tex.x));
    optixSetPayload_9(__float_as_uint(tex.y));
    optixSetPayload_10(__float_as_uint(tex.z));

    optixSetPayload_17(1u); // exit
}

extern "C" __global__ void __closesthit__ch()
{
    const int primIdx = optixGetPrimitiveIndex();

    const float3 rayDir = optixGetWorldRayDirection();
    const float3 rayO = optixGetWorldRayOrigin();

    HitData data{};
    data.t = optixGetRayTmax();
    data.triangle_id = primIdx;
    const TriangleData& tri = params.geometry[primIdx].triangle;
    const float3 N_0 = tri.normalFlat;
    data.geo_normal = faceforward(N_0, -rayDir, N_0);
    if (!tri.hasVertexNormals) {
        data.shading_normal = data.geo_normal;
    }
    else {
        const float2 bary = optixGetTriangleBarycentrics();
        const float u = bary.x;
        const float v = bary.y;
        const float w = 1.0f - u - v;
        data.shading_normal = normalize(w * tri.n0 + u * tri.n1 + v * tri.n2);
        data.shading_normal = 
            faceforward(data.shading_normal, data.geo_normal, data.shading_normal);
    }
    data.front_face = dot(N_0, data.geo_normal) > 0.0f;
    data.pos = rayO + data.t * rayDir;

    const Material& mat = params.materials[params.geometry[primIdx].materialIndex];

    RadiancePRD prd = {};
    prd.rayO = rayO;
    prd.rayDir = rayDir;
    prd.radiance = make_float3(0.0f);
    prd.seed = optixGetPayload_3();
    prd.depth = optixGetPayload_4();
    prd.prevHitPos = make_float3(
        __uint_as_float(optixGetPayload_5()), 
        __uint_as_float(optixGetPayload_6()), 
        __uint_as_float(optixGetPayload_7()));
    prd.prevDelta = optixGetPayload_18();
    prd.prevScatterPdf = __uint_as_float(optixGetPayload_19());

    // shading
    shadePT(prd, data, mat);


    optixSetPayload_0(__float_as_uint(prd.attenuation.x));
    optixSetPayload_1(__float_as_uint(prd.attenuation.y));
    optixSetPayload_2(__float_as_uint(prd.attenuation.z));
    optixSetPayload_3(prd.seed);
    //optixSetPayload_4(prd.depth);
    optixSetPayload_5(__float_as_uint(prd.prevHitPos.x));
    optixSetPayload_6(__float_as_uint(prd.prevHitPos.y));
    optixSetPayload_7(__float_as_uint(prd.prevHitPos.z));
    optixSetPayload_8(__float_as_uint(prd.radiance.x)); 
    optixSetPayload_9(__float_as_uint(prd.radiance.y));
    optixSetPayload_10(__float_as_uint(prd.radiance.z));
    optixSetPayload_11(__float_as_uint(prd.rayO.x));
    optixSetPayload_12(__float_as_uint(prd.rayO.y));
    optixSetPayload_13(__float_as_uint(prd.rayO.z));
    optixSetPayload_14(__float_as_uint(prd.rayDir.x));
    optixSetPayload_15(__float_as_uint(prd.rayDir.y));
    optixSetPayload_16(__float_as_uint(prd.rayDir.z));
    optixSetPayload_17(prd.exit);
    optixSetPayload_18(prd.prevDelta);
    optixSetPayload_19(__float_as_uint(prd.prevScatterPdf));
}