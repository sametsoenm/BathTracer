#pragma once

#include "device_types.h"
#include "material_funcs.h"
#include "util/vec_math.h"
#include "util/sampling.h"
#include "util/onb.h"

__forceinline__ __device__ float pdf_specular_microfacet(
    const float3& wo,
    const float3& wi,
    const Material& mat,
    const HitData& data) {

    Onb onb(data.geo_normal);
    const float3 wo_local = onb.to_local(wo);
    const float3 wi_local = onb.to_local(wi);
    const float3 wh_local = normalize(wo_local + wi_local);

    const float alpha = mat.alpha;
    const float G1 = material::G1_smith(alpha, wo_local, wh_local);
    const float D = material::NDF_GGX(alpha, wh_local);
    const float wo_dot_wh = dot(wo_local, wh_local);
    const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

    return G1 * wo_dot_wh_clamp * D / (4.0f * wo_dot_wh * wo_local.z);
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_specular_microfacet(
    const float3& wo,
    const float3& wi,
    const Material& mat,
    const HitData& data) {

    const float3 N = data.shading_normal;

    Onb onb(N);
    const float3 wo_local = onb.to_local(wo);
    const float3 wi_local = onb.to_local(wi);

    if (wi_local.z <= 0.0f) {
        return make_float3(0.0f);
    }

    const float3 wh_local = normalize(wo_local + wi_local);
    const float wo_dot_wh = max(0.0f, dot(wo_local, wh_local));

    const float3 f0 = mat.color;
    const float alpha = mat.alpha;
    const float3 F = material::fresnel_schlick(f0, wo_dot_wh);
    const float D = material::NDF_GGX(alpha, wh_local);
    const float G = material::G_smith(alpha, wi_local, wo_local, wh_local);

    const float V = dot(data.geo_normal, wi) <= 0.0f ? 0.0f : 1.0f; // necessary if shading normal differs from geometric normal

    return clamp(V * F * D * G / (4.0f * wo_local.z), 0.0f, 1e36f); // todo: find reason for NaNs instead of clamping
}

__forceinline__ __device__ BSDFSample sample_specular_microfacet(
	const float3& wo,
	const Material& mat,
	const HitData& data,
	const float2& u) {

    BSDFSample s{};

    Onb onb(data.geo_normal);
    const float3 wo_local = onb.to_local(wo);
    const float alpha = mat.alpha;
    const float3 wh_local = sampling::sampleGGXVNDF(alpha, wo_local, u);
    const float3 wi_local = reflect(-wo_local, wh_local);

    const float G1 = material::G1_smith(alpha, wo_local, wh_local);
    const float D = material::NDF_GGX(alpha, wh_local);
    const float wo_dot_wh = dot(wo_local, wh_local);
    const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

    s.pdf = G1 * wo_dot_wh_clamp * D / (4.0f * wo_dot_wh * wo_local.z);

    float3 wi = onb.to_world(wi_local);

    s.is_delta = false;
    s.wi = wi;
    s.contrib = eval_specular_microfacet(wo, wi, mat, data) / s.pdf;

    return s;
}