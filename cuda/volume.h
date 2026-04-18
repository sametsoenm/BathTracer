#pragma once

#include "device_types.h"
#include "light.h"
#include "util/vec_math.h"
#include "util/random.h"
#include "util/sampling.h"
#include "util/util.h"

#include "rendersettings.h"

__forceinline__ __device__ void volume_interaction_single(RadiancePRD& prd, float t_sample) {

	const float3 P = prd.rayO + t_sample * prd.rayDir;
	unsigned int seed = prd.seed;

	{ // NEE
		LightSample l = sample_lights(
			params.lights, params.numLights, params.materials, P, rnd3(seed));

		const float3 wl = normalize(l.pos - P);
		const float d = length(l.pos - P);
		const float T = expf(-params.fogDensity * d);

		const bool occluded =
			isOccluded(
				params.traversable,
				P,
				wl,
				settings::T_MIN,
				d - 0.001f);
		const float V = occluded ? 0.0f : 1.0f;

		const float phase_func = sampling::eval_isotropic_phase_func();

		prd.radiance = V * T * phase_func * l.emission / l.pdf; // need to add mu_s for pbr
		prd.exit = true;
		prd.seed = seed;
	}
}

__forceinline__ __device__ void volume_interaction_multiple_PT(RadiancePRD& prd, float t_sample) {

	const float3 P = prd.rayO + t_sample * prd.rayDir;
	unsigned int seed = prd.seed;

	// sample new direction
	float3 wi = sampling::sample_uniform_sphere(rnd2(seed));

	prd.attenuation = make_float3(1.0f);
	prd.prevDelta = false;
	prd.seed = seed;
	prd.exit = 0u;

	prd.rayO = P;
	prd.rayDir = wi;
}

__forceinline__ __device__ void volume_interaction_multiple_NEE(RadiancePRD& prd, float t_sample) {

	const float3 P = prd.rayO + t_sample * prd.rayDir;
	unsigned int seed = prd.seed;

	{ // NEE
		LightSample l = sample_lights(
			params.lights, params.numLights, params.materials, P, rnd3(seed));

		const float3 wl = normalize(l.pos - P);
		const float d = length(l.pos - P);
		const float T = expf(-params.fogDensity * d);

		const bool occluded =
			isOccluded(
				params.traversable,
				P,
				wl,
				settings::T_MIN,
				d - 0.001f);
		const float V = occluded ? 0.0f : 1.0f;

		const float phase_func = sampling::eval_isotropic_phase_func();

		prd.radiance = V * T * params.fogDensity * phase_func * l.emission / l.pdf;
	}

	// sample new direction
	float3 wi = sampling::sample_uniform_sphere(rnd2(seed));

	prd.attenuation = make_float3(1.0f);
	prd.prevDelta = false;
	prd.seed = seed;
	prd.exit = 0u;

	prd.rayO = P;
	prd.rayDir = wi;
}

__forceinline__ __device__ void volume_interaction_multiple_MIS(RadiancePRD& prd, float t_sample) {

	const float3 P = prd.rayO + t_sample * prd.rayDir;
	unsigned int seed = prd.seed;

	{ // NEE
		LightSample l = sample_lights(
			params.lights, params.numLights, params.materials, P, rnd3(seed));

		const float3 wl = normalize(l.pos - P);
		const float d = length(l.pos - P);
		const float T = expf(-params.fogDensity * d);

		const bool occluded =
			isOccluded(
				params.traversable,
				P,
				wl,
				settings::T_MIN,
				d - 0.001f);
		const float V = occluded ? 0.0f : 1.0f;

		const float phase_func = sampling::eval_isotropic_phase_func();

		const float pdf_NEE = l.pdf;
		const float pdf_phase = sampling::pdf_isotropic_phase_func();
		const float weight = l.isHittable
			? sampling::power_heuristic(pdf_NEE, pdf_phase)
			: 1.0f;

		prd.radiance = V * weight * T * params.fogDensity * phase_func * l.emission / pdf_NEE;
	}

	// sample new direction
	float3 wi = sampling::sample_uniform_sphere(rnd2(seed));

	prd.attenuation = make_float3(1.0f);
	prd.prevDelta = false;
	prd.seed = seed;
	prd.prevHitPos = P;
	prd.exit = 0u;
	prd.prevScatterPdf = sampling::pdf_isotropic_phase_func();

	prd.rayO = P;
	prd.rayDir = wi;
}


__forceinline__ __device__ void volumeEvent(FogMode fogMode, RadiancePRD& prd, float t_sample) {
	switch (fogMode) {
	case FogMode::SINGLE_SCATTER:
		volume_interaction_single(prd, t_sample);
		break;
	case FogMode::MULTIPLE_SCATTER:
		volume_interaction_multiple_PT(prd, t_sample);
		break;
	default:
		volume_interaction_single(prd, t_sample);
	}
}