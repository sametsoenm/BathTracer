#pragma once

#include "device_types.h"
#include "util/vec_math.h"
#include "util/sampling.h"
#include "util/onb.h"

__forceinline__ __device__ float pdf_mirror() {

	return 0.0f;
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_mirror() {

	return make_float3(0.0f);
}

__forceinline__ __device__ BSDFSample sample_mirror(
	const float3& wo,
	const Material& mat,
	const HitData& data) {

	BSDFSample s{};

	const float3 wi = reflect(-wo, data.shading_normal);

	s.contrib = mat.reflectance;
	s.pdf = 1.0f;
	s.wi = wi;

	return s;
}