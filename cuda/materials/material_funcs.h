#pragma once

#include "util/vec_math.h"

namespace material {

	__forceinline__ __device__ float3 fresnel_schlick(const float3& f0, const float cos_theta) {
		return f0 + (1.0f - f0) * pow((1.0f - cos_theta), 5.0f);
	}

	__forceinline__ __device__ float fresnel_dielectric(
		const float eta1, const float eta2, const float cosr, const float cost) {

		const auto Rs = (eta1 * cosr - eta2 * cost) / (eta1 * cosr + eta2 * cost);
		const auto Rp = (eta1 * cost - eta2 * cosr) / (eta1 * cost + eta2 * cosr);
		return (Rs * Rs + Rp * Rp) * 0.5f;
	}

	__forceinline__ __device__ float3 refract(
		const float3& I, const float3& N, const float etaI, const float etaT) {
		const float eta = etaI / etaT;
		const float k = 1.f - eta * eta * (1.f - dot(N, I) * dot(N, I));
		return k < 0.f ? make_float3(0.f) : eta * I - (eta * dot(N, I) + sqrtf(k)) * N;
	}

	__forceinline__ __device__ float G1_smith(const float alpha, const float3& wx, const float3& wh) {
		//if (dot(wx, wh) <= 0.0f) // if active transmission wont work
		//	return 0.0f;

		const float a2 = alpha * alpha;
		const float frac = (wx.x * wx.x + wx.y * wx.y) / (wx.z * wx.z);
		const float lambda = (-1.0f + sqrtf(1 + a2 * frac)) * 0.5f;
		return 1.0f / (1.0f + lambda);
	}

	__forceinline__ __device__ float NDF_GGX(const float alpha, const float3& wh) {
		if (wh.z <= 0.0f) // cos_theta assuming N is (0,0,1)
			return 0.0f;
		const float a2 = alpha * alpha;
		const float denom_base = wh.x * wh.x + wh.y * wh.y + a2 * wh.z * wh.z;
		return a2 / (M_PIf * denom_base * denom_base);
	}

	__forceinline__ __device__ float G_smith(const float alpha,
		const float3& wi, const float3& wo, const float3& wh) {
		return G1_smith(alpha, wi, wh) * G1_smith(alpha, wo, wh);
	}

}