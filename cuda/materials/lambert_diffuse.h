#pragma once

#include "device_types.h"
#include "util/vec_math.h"
#include "util/sampling.h"
#include "util/onb.h"

#include <stdio.h>

__forceinline__ __device__ float pdf_lambert_diffuse(
	const float3& wi, 
	const HitData& data) {

	return sampling::pdf_cosine_hemisphere( dot(wi, data.geo_normal) );
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_lambert_diffuse(
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	const float3 N = data.shading_normal;
	if (dot(wi, N) <= 0.0f)
		return make_float3(0.0f);

	const float cosTheta = dot(data.geo_normal, wi) > 0.0f
		? max(0.0f, dot(N, wi)) : 0.0f;

	return (mat.color / M_PIf) * cosTheta;
}

__forceinline__ __device__ BSDFSample sample_lambert_diffuse(
	const Material& mat, 
	const HitData& data,
	const float2& u) {

	BSDFSample s{};

	// sample locally
	float3 wi = sampling::sample_cosine_hemisphere(u);
	Onb onb(data.geo_normal);
	// transform to world space
	onb.to_world_inplace(wi);

	const float3 N = data.shading_normal;

	if (dot(wi, N) <= 0.0f) {
		s.wi = wi;
		s.pdf = 0.0f;
		s.contrib = make_float3(0.0f);
		return s;
	}
	//printf("%d, %d, %d", mat.color.x, mat.color.y, mat.color.z); 
	s.contrib = mat.color;
	s.pdf = pdf_lambert_diffuse(wi, data);
	s.wi = wi;

	return s;
}