#pragma once

#include "device_types.h"
#include "light.h"
#include "util/vec_math.h"
#include "util/random.h"
#include "util/sampling.h"
#include "util/util.h"

#include "rendersettings.h"

// unidirectional path tracing with MIS
__forceinline__ __device__ void shadeMIS(RadiancePRD& prd, HitData& data, const Material& mat) {

    unsigned int seed = prd.seed;
    const float T = params.fogEnabled ? expf(-params.fogDensity * data.t) : 1.0f;

    if (mat.type == MaterialType::EMISSIVE_DIFFUSE) {
        if (prd.depth == 0 || prd.prevDelta) {
            prd.radiance = mat.emission;
        }
        else {

            const float pdf_bsdf = prd.prevScatterPdf;
            const TriangleData& tri = params.geometry[data.triangle_id].triangle;
            const float3 e1 = tri.v1 - tri.v0;
            const float3 e2 = tri.v2 - tri.v0;
            const float pdf_light = 
                sampling::pdf_uniform_triangle(e1, e2, data.geo_normal, prd.prevHitPos, data.pos);

            const float weight = sampling::power_heuristic(pdf_bsdf, pdf_light);
            prd.radiance = T * weight * mat.emission; // is T right here?
        }
        prd.exit = 1u;
        return;
    }

    // NEE
    {
        LightSample l = sample_lights(
            params.lights, params.numLights, params.materials, data.pos, rnd3(seed));

        const float3 wl = normalize(l.pos - data.pos);
        const float d = length(l.pos - data.pos);

        const bool occluded =
            isOccluded(
                params.traversable,
                data.pos,
                wl,
                settings::T_MIN,
                d - 0.001f);
        const float V = occluded ? 0.0f : 1.0f;
        float3 bsdf_cos = eval_material(-prd.rayDir, wl, mat, data);
        const float T_NEE = params.fogEnabled ? expf(-params.fogDensity * d) : 1.0f;

        const float pdf_NEE = l.pdf;
        const float pdf_bsdf = pdf_material(-prd.rayDir, wl, mat, data);
        const float weight = l.isHittable 
            ? sampling::power_heuristic(pdf_NEE, pdf_bsdf) 
            : 1.0f;

        prd.radiance = V * weight * T * T_NEE * bsdf_cos * l.emission / l.pdf;
    }

    BSDFSample sample = sample_material(-prd.rayDir, mat, data, rnd3(seed));

    prd.rayDir = sample.wi;
    prd.rayO = data.pos;
    prd.attenuation = T * sample.contrib;
    prd.prevDelta = mat.isDelta;
    prd.seed = seed;
    prd.prevHitPos = data.pos;
    prd.exit = 0u;
    prd.prevScatterPdf = sample.pdf;

}

// unidirectional path tracing with NEE
__forceinline__ __device__ void shadeNEE(RadiancePRD& prd, HitData& data, const Material& mat) {

    unsigned int seed = prd.seed;

    if (mat.type == MaterialType::EMISSIVE_DIFFUSE) {
        if (prd.depth == 0 || prd.prevDelta) {
            prd.radiance = mat.emission;
        }
        else {
            prd.radiance = make_float3(0.0f);
        }
        prd.exit = 1u;
        return;
    }

    const float T = params.fogEnabled ? expf(-params.fogDensity * data.t) : 1.0f;

    // NEE
    {
        LightSample l = sample_lights(
            params.lights, params.numLights, params.materials, data.pos, rnd3(seed));

        const float3 wl = normalize(l.pos - data.pos);
        const float d = length(l.pos - data.pos);

        const bool occluded =
            isOccluded(
                params.traversable,
                data.pos,
                wl,
                settings::T_MIN,
                d - 0.001f);
        const float V = occluded ? 0.0f : 1.0f;
        float3 bsdf_cos = eval_material(-prd.rayDir, wl, mat, data);
        const float T_NEE = params.fogEnabled ? expf(-params.fogDensity * d) : 1.0f;

        prd.radiance = V * T * T_NEE * bsdf_cos * l.emission / l.pdf;
    }

    BSDFSample sample = sample_material(-prd.rayDir, mat, data, rnd3(seed));

    prd.rayDir = sample.wi;
    prd.rayO = data.pos ;
    prd.attenuation = T * sample.contrib;
    prd.prevDelta = mat.isDelta;
    prd.seed = seed;
    prd.exit = 0u;

}

// unidirectional path tracing
__forceinline__ __device__ void shadePT(RadiancePRD& prd, HitData& data, const Material& mat) {

    const float T = params.fogEnabled ? expf(-params.fogDensity * data.t) : 1.0f;
    unsigned int seed = prd.seed;

    if (mat.type == MaterialType::EMISSIVE_DIFFUSE) {
        if (prd.depth == 0) {
            prd.radiance = mat.emission;
        }
        else {
            prd.radiance = data.front_face
                ? T * mat.emission
                : make_float3(0.0f);
        }

        prd.exit = 1u;
        return;
    }

    BSDFSample sample = sample_material(-prd.rayDir, mat, data, rnd3(seed));

    prd.rayDir = sample.wi;
    prd.rayO = data.pos;
    prd.attenuation = T * sample.contrib;
    prd.seed = seed;
    prd.exit = 0u;
}