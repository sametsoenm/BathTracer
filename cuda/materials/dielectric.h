#pragma once

#include "device_types.h"
#include "material_funcs.h"
#include "util/vec_math.h"
#include "util/sampling.h"
#include "util/onb.h"

#include <stdio.h>

__forceinline__ __device__ float pdf_smooth_dielectric() {

	return 0.0f;
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_smooth_dielectric() {

	return make_float3(0.0f);
}

__forceinline__ __device__ BSDFSample sample_smooth_dielectric(
	const float3& wo,
	const Material& mat,
	const HitData& data,
	const float u) {

	BSDFSample s{};

	float etaI = 1.0f; // air IOR
	float etaJ = mat.eta;
	if (!data.front_face) {
		etaI = mat.eta;
		etaJ = 1.0f;
	}

	const float cosr = max(0.0f, dot(wo, data.shading_normal));
	const float eta = etaI / etaJ;
	const float sint2 = eta * eta * (1.0f - cosr * cosr);

	// check for total internal reflection
	if (sint2 > 1.0f) {
		s.wi = reflect(-wo, data.shading_normal);
		s.pdf = 1.0f; // maybe adjust for NEE
		s.contrib = make_float3(1.0f); // pdf and bsdf not math accurate
		s.sign = -1.0f;
		return s;
	}

	const float cost = sqrtf(1.0f - sint2);
	const float F = material::fresnel_dielectric(etaI, etaJ, cosr, cost);

	// reflection
	if (u < F) {
		s.wi = reflect(-wo, data.shading_normal);
		s.pdf = 1.0f;
		s.contrib = make_float3(1.0f);
		return s;
	}

	// transmission
	s.wi = normalize(material::refract(-wo, data.shading_normal, etaI, etaJ));
	s.pdf = 1.0f;
	const float weight = etaJ * etaJ / etaI / etaI;
	s.contrib = make_float3(weight);
	s.sign = -1.0f;

	return s;
}


__forceinline__ __device__ float pdf_rough_dielectric(
    const float3& wo,
    const float3& wi,
    const Material& mat,
    const HitData& data) {

    float etaI = 1.0f; // air IOR
    float etaJ = mat.eta;
    if (!data.front_face) {
        etaI = mat.eta;
        etaJ = 1.0f;
    }

    const float3 N = data.geo_normal;
    Onb onb(N);
    const float3 wo_local = onb.to_local(wo);
    const float3 wi_local = onb.to_local(wi);

    const float alpha = mat.alpha;
    const bool reflect = wo_local.z * wi_local.z > 0.0f;

    if (reflect) {
        float3 wh_local = normalize(wo_local + wi_local);
        const float G1 = material::G1_smith(alpha, wo_local, wh_local);
        const float D = material::NDF_GGX(alpha, wh_local);
        const float wo_dot_wh = dot(wo_local, wh_local);
        const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

        const float cosr = max(0.0f, dot(wo_local, wh_local));
        const float eta = etaI / etaJ;
        const float sint2 = eta * eta * (1.0f - cosr * cosr);
        const float cost = sqrtf(1.0f - sint2);
        const float F = sint2 < 1.0f
            ? material::fresnel_dielectric(etaI, etaJ, cosr, cost) : 1.0f;

        return G1 * wo_dot_wh_clamp * D / (4.0f * wo_dot_wh * wo_local.z) * F;

    }

    // transmission
    float3 wh_local = normalize((etaJ * wi_local + etaI * wo_local)); // not 100% sure but probably cordatat

    if (dot(wo_local, wh_local) <= 0.0f)
        return 0.0f; // cordatat?

    const float cosr = max(0.0f, dot(wo_local, wh_local));
    const float eta = etaI / etaJ;
    const float sint2 = eta * eta * (1.0f - cosr * cosr);
    const float cost = sqrtf(1.0f - sint2);
    const float F = sint2 < 1.0f
        ? material::fresnel_dielectric(etaI, etaJ, cosr, cost) : 1.0f;

    const float G1 = material::G1_smith(alpha, wo_local, wh_local);
    const float D = material::NDF_GGX(alpha, wh_local);
    const float wo_dot_wh = dot(wo_local, wh_local);
    const float wi_dot_wh = dot(wi_local, wh_local); // abs?
    const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

    const float denom_sqrt = wi_dot_wh + wo_dot_wh / (etaJ / etaI);
    const float jacobian = fabs(wo_dot_wh) / (denom_sqrt * denom_sqrt);

    return (1.0f - F) * G1 * fabs(wo_dot_wh) * D / fabs(wo_local.z) * jacobian;
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_rough_dielectric(
    const float3& wo,
    const float3& wi,
    const Material& mat,
    const HitData& data) {

    float etaI = 1.0f; // air IOR
    float etaJ = mat.eta;
    if (!data.front_face) {
        etaI = mat.eta;
        etaJ = 1.0f;
    }

    const float3 N = data.geo_normal;
    Onb onb(N);
    const float3 wo_local = onb.to_local(wo);
    const float3 wi_local = onb.to_local(wi);

    const float alpha = mat.alpha;
    const bool reflect = wo_local.z * wi_local.z > 0.0f;

    if (reflect) {
        float3 wh_local = normalize(wo_local + wi_local);

        const float cosr = max(0.0f, dot(wo_local, wh_local));
        const float eta = etaI / etaJ;
        const float sint2 = eta * eta * (1.0f - cosr * cosr);
        const float cost = sqrtf(1.0f - sint2);
        const float F = sint2 < 1.0f
            ? material::fresnel_dielectric(etaI, etaJ, cosr, cost) : 1.0f;

        const float wo_dot_wh = max(0.0f, dot(wo_local, wh_local));
        const float D = material::NDF_GGX(alpha, wh_local);
        const float G = material::G_smith(alpha, wi_local, wo_local, wh_local);

        return make_float3(F * D * G / (4.0f * wo_local.z));
    }

    // trnasmission
    float3 wh_local = normalize((etaJ * wi_local + etaI * wo_local)); // not 100% sure but probably cordatat

    if (dot(wo_local, wh_local) <= 0.0f)
        return make_float3(0.0f); // cordatat?

    const float cosr = max(0.0f, dot(wo_local, wh_local));
    const float eta = etaI / etaJ;
    const float sint2 = eta * eta * (1.0f - cosr * cosr);
    const float cost = sqrtf(1.0f - sint2);
    const float F = sint2 < 1.0f
        ? material::fresnel_dielectric(etaI, etaJ, cosr, cost) : 1.0f;
    const float D = material::NDF_GGX(alpha, wh_local);
    const float G = material::G_smith(alpha, wi_local, wo_local, wh_local);
    const float wo_dot_wh = dot(wo_local, wh_local);
    const float wi_dot_wh = dot(wi_local, wh_local); // abs?
    const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

    const float denom_sqrt = wi_dot_wh + wo_dot_wh / (etaJ / etaI);

    float bsdf_cos = (1.0f - F) * D * G * fabs(wi_dot_wh) * fabs(wo_dot_wh)
        / (denom_sqrt * denom_sqrt * fabs(wo_local.z));
    bsdf_cos *= etaJ * etaJ / etaI / etaI;

    return make_float3(bsdf_cos);
}

__forceinline__ __device__ BSDFSample sample_rough_dielectric(
    const float3& wo,
    const Material& mat,
    const HitData& data,
    const float3& u) {

    BSDFSample s{};

    float etaI = 1.0f; // air IOR
    float etaJ = mat.eta;
    if (!data.front_face) {
        etaI = mat.eta;
        etaJ = 1.0f;
    }

    Onb onb(data.geo_normal);
    const float3 wo_local = onb.to_local(wo);

    const float alpha = mat.alpha;
    const float3 wh_local = sampling::sampleGGXVNDF(alpha, wo_local, make_float2(u));

    const float cosr = max(0.0f, dot(wo_local, wh_local));
    const float eta = etaI / etaJ;
    const float sint2 = eta * eta * (1.0f - cosr * cosr);
    const float cost = sqrtf(1.0f - sint2);

    float F = 0.0f;
    // check for total internal reflection
    if (sint2 >= 1.0f) {
        s.sign = -1.0f;
        F = 1.0f;
    }
    else {
        F = material::fresnel_dielectric(etaI, etaJ, cosr, cost);
    }

    // reflection
    if (u.z < F) {

        const float3 wi_local = reflect(-wo_local, wh_local);

        const float G1 = material::G1_smith(alpha, wo_local, wh_local);
        const float D = material::NDF_GGX(alpha, wh_local);
        const float wo_dot_wh = dot(wo_local, wh_local);
        const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

        float3 wi = onb.to_world(wi_local);
        s.wi = wi;
        s.pdf = G1 * fabs(wo_dot_wh) * D / (4.0f * fabs(wo_dot_wh) * wo_local.z) * F;

        // bsdf_cos
        if (wi_local.z <= 0.0f) {
            s.pdf = 0.0f;
            s.contrib = make_float3(0.0f);
            return s;
        }

        const float G = material::G_smith(alpha, wi_local, wo_local, wh_local);
        s.contrib = make_float3(F * G * wo_dot_wh / (G1 * wo_dot_wh_clamp));
        return s;
    }

    // transmission
    const float T = 1.0f - F;
    s.sign = -1.0f;

    const float3 wi_local = material::refract(-wo_local, wh_local, etaI, etaJ);

    if (wo_local.z * wi_local.z > 0.0f) { // check same hemisphere
        s.pdf = 0.0f;
        s.contrib = make_float3(0.0f);
    }

    const float G1 = material::G1_smith(alpha, wo_local, wh_local);
    const float D = material::NDF_GGX(alpha, wh_local);
    const float wo_dot_wh = dot(wo_local, wh_local);
    const float wi_dot_wh = dot(wi_local, wh_local); // abs?
    const float wo_dot_wh_clamp = max(0.0f, wo_dot_wh);

    const float denom_sqrt = wi_dot_wh + wo_dot_wh / (etaJ / etaI);
    const float jacobian = fabs(wo_dot_wh) / (denom_sqrt * denom_sqrt);

    s.pdf = G1 * fabs(wo_dot_wh) * D / fabs(wo_local.z) * jacobian * T;

    const float G = material::G_smith(alpha, wi_local, wo_local, wh_local);
    float bsdf_cos = T * D * G * fabs(wi_dot_wh) * fabs(wo_dot_wh)
        / (denom_sqrt * denom_sqrt * fabs(wo_local.z));

    bsdf_cos *= etaJ * etaJ / etaI / etaI;

    /*float3 color = make_float3(0.2f, 0.2f, 0.2f);
    float3 transmit = make_float3(1.0f);
    if (!data.front_face) {
        transmit = make_float3(
            expf(-data.t * color.x),
            expf(-data.t * color.y),
            expf(-data.t * color.z)
        );
    }*/

    float3 wi = onb.to_world(wi_local);
    s.wi = wi;
    s.contrib = /*transmit **/ make_float3(bsdf_cos / s.pdf);
    return s;
}