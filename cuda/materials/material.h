#pragma once

#include "lambert_diffuse.h"
#include "mirror.h"
#include "dielectric.h"
#include "specular_microfacet.h"

__forceinline__ __device__ BSDFSample sample_material(
	const float3& wo,
	const Material& mat,
	const HitData& data,
	const float3& u) {

	switch (mat.type) {
	case MaterialType::LAMBERT_DIFFUSE:
		return sample_lambert_diffuse(mat, data, make_float2(u));
	case MaterialType::MIRROR:
		return sample_mirror(wo, mat, data);
	case MaterialType::SPECULAR_MICROFACET:
		return sample_specular_microfacet(wo, mat, data, make_float2(u));
	case MaterialType::SMOOTH_DIELECTRIC:
		return sample_smooth_dielectric(wo, mat, data, u.x);
	case MaterialType::ROUGH_DIELECTRIC:
		return sample_rough_dielectric(wo, mat, data, u);
	default:
		return BSDFSample{};
	}
}

__forceinline__ __device__ float pdf_material(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	switch (mat.type) {
	case MaterialType::LAMBERT_DIFFUSE:
		return pdf_lambert_diffuse(wi, data);
	case MaterialType::MIRROR:
		return pdf_mirror();
	case MaterialType::SPECULAR_MICROFACET:
		return pdf_specular_microfacet(wo, wi, mat, data);
	case MaterialType::SMOOTH_DIELECTRIC:
		return pdf_smooth_dielectric();
	case MaterialType::ROUGH_DIELECTRIC:
		return pdf_rough_dielectric(wo, wi, mat, data);
	default:
		return 0.0f;
	}
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_material(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	switch (mat.type) {
	case MaterialType::LAMBERT_DIFFUSE:
		return eval_lambert_diffuse(wi, mat, data);
	case MaterialType::MIRROR:
		return eval_mirror();
	case MaterialType::SPECULAR_MICROFACET:
		return eval_specular_microfacet(wo, wi, mat, data);
	case MaterialType::SMOOTH_DIELECTRIC:
		return eval_smooth_dielectric();
	case MaterialType::ROUGH_DIELECTRIC:
		return eval_rough_dielectric(wo, wi, mat, data);
	default:
		return make_float3(0.0f);
	}
}