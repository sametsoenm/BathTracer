#pragma once

#include "util/vec_math.h"

namespace material {

	__forceinline__ __device__ float3 fresnel_schlick(const float3& f0, const float cos_theta) {
		return f0 + (1.0f - f0) * powf((1.0f - cos_theta), 5.0f);
	}

	__forceinline__ __device__ float fresnel_schlick(const float f0, const float cos_theta) {
		return f0 + (1.0f - f0) * powf((1.0f - cos_theta), 5.0f);
	}

	__forceinline__ __device__ float fresnel_dielectric(
		const float eta1, const float eta2, const float cosr, const float cost) {

		const auto Rs = (eta1 * cosr - eta2 * cost) / (eta1 * cosr + eta2 * cost);
		const auto Rp = (eta1 * cost - eta2 * cosr) / (eta1 * cost + eta2 * cosr);
		return (Rs * Rs + Rp * Rp) * 0.5f;
	}

	__forceinline__ __device__ float fresnel_dielectric(
		const float cosThetaI,
		const float etaI,
		const float etaT) {

		const float cosI = clamp(fabsf(cosThetaI), 0.0f, 1.0f);
		const float eta = etaI / etaT;
		const float sinT2 = eta * eta * (1.0f - cosI * cosI);

		if (sinT2 >= 1.0f)
			return 1.0f;

		const float cosT = sqrtf(1.0f - sinT2);
		return fresnel_dielectric(etaI, etaT, cosI, cosT);
	}

	__forceinline__ __device__ float schlick_R0_from_relative_ior(const float relativeIor) {
		const float r = (relativeIor - 1.0f) / (relativeIor + 1.0f);
		return r * r;
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

	__forceinline__ __device__ float GTR1_burley(const float alpha, const float cos_theta) {
		const float a2 = alpha * alpha;
		const float denom = M_PIf * logf(a2) * (1.0f + (a2 - 1.0f) * cos_theta * cos_theta);
		return (a2 - 1.0f) / denom;
	}

	__forceinline__ __device__ void anisotropic_params_burley(
		const float roughness,
		const float anisotropic,
		float& ax,
		float& ay) {

		const float aspect = sqrtf(1.0f - 0.9f * anisotropic);
		const float roughness2 = roughness * roughness;
		ax = fmaxf(0.001f, roughness2 / aspect);
		ay = fmaxf(0.001f, roughness2 * aspect);
	}

	__forceinline__ __device__ float NDF_GTR2_anisotropic(
		const float ax,
		const float ay,
		const float3& wh) {

		if (wh.z <= 0.0f)
			return 0.0f;

		const float x = wh.x / ax;
		const float y = wh.y / ay;
		const float denom = x * x + y * y + wh.z * wh.z;
		return 1.0f / (M_PIf * ax * ay * denom * denom);
	}

	__forceinline__ __device__ float G1_smith_anisotropic(
		const float ax,
		const float ay,
		const float3& w,
		const float3& wh) {

		if (dot(w, wh) <= 0.0f || w.z <= 0.0f)
			return 0.0f;

		const float xy2 = w.x * w.x + w.y * w.y;
		if (xy2 <= 0.0f)
			return 1.0f;

		const float tan2Theta = xy2 / (w.z * w.z);
		const float cos2Phi = w.x * w.x / xy2;
		const float sin2Phi = w.y * w.y / xy2;
		const float alpha2 = cos2Phi * ax * ax + sin2Phi * ay * ay;
		const float lambda = (-1.0f + sqrtf(1.0f + alpha2 * tan2Theta)) * 0.5f;
		return 1.0f / (1.0f + lambda);
	}

	__forceinline__ __device__ float G1_smith_anisotropic_abs(
		const float ax,
		const float ay,
		const float3& w,
		const float3& wh) {

		if (dot(w, wh) == 0.0f || w.z == 0.0f)
			return 0.0f;

		const float xy2 = w.x * w.x + w.y * w.y;
		if (xy2 <= 0.0f)
			return 1.0f;

		const float z = fabsf(w.z);
		const float tan2Theta = xy2 / (z * z);
		const float cos2Phi = w.x * w.x / xy2;
		const float sin2Phi = w.y * w.y / xy2;
		const float alpha2 = cos2Phi * ax * ax + sin2Phi * ay * ay;
		const float lambda = (-1.0f + sqrtf(1.0f + alpha2 * tan2Theta)) * 0.5f;
		return 1.0f / (1.0f + lambda);
	}

	__forceinline__ __device__ float G_smith_anisotropic_abs(
		const float ax,
		const float ay,
		const float3& wi,
		const float3& wo,
		const float3& wh) {

		return G1_smith_anisotropic_abs(ax, ay, wi, wh) * G1_smith_anisotropic_abs(ax, ay, wo, wh);
	}

	__forceinline__ __device__ float G_smith_anisotropic(
		const float ax,
		const float ay,
		const float3& wi,
		const float3& wo,
		const float3& wh) {

		return G1_smith_anisotropic(ax, ay, wi, wh) * G1_smith_anisotropic(ax, ay, wo, wh);
	}

}
