#pragma once

#include "device_types.h"
#include "util/vec_math.h"

struct DisneySurface {
	float3 baseColor;
	float3 transmittanceColor;

	float metallic;
	float roughness;
	float specular;
	float specularTint;
	float anisotropic;
	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;
	float specTrans;
	float diffTrans;
	float flatness;
	float eta;
	float scatterDistance;
	bool thin;
};

struct DisneyLobePdfs {
	float specular;
	float diffuse;
	float clearcoat;
	float specTransmission;
};

__forceinline__ __device__ DisneySurface make_disney_surface(
	const Material& mat,
	const HitData& data) {

	DisneySurface surface{};
	surface.baseColor = make_float3(
		tex2D<float4>(params.textures[mat.colorTexIdx], data.uv.x, data.uv.y)
	);
	surface.transmittanceColor = mat.transmittanceColor;
	surface.metallic = mat.metallic;
	surface.roughness = mat.roughness;
	surface.specular = mat.specular;
	surface.specularTint = mat.specularTint;
	surface.anisotropic = mat.anisotropic;
	surface.sheen = mat.sheen;
	surface.sheenTint = mat.sheenTint;
	surface.clearcoat = mat.clearcoat;
	surface.clearcoatGloss = mat.clearcoatGloss;
	surface.specTrans = mat.specTrans;
	surface.diffTrans = mat.diffTrans;
	surface.flatness = mat.flatness;
	surface.eta = mat.eta;
	surface.scatterDistance = mat.scatterDistance;
	surface.thin = mat.thin;
	return surface;
}

__forceinline__ __device__ DisneyLobePdfs calculate_disney_lobe_pdfs(
	const DisneySurface& surface) {

	(void)surface;
	return DisneyLobePdfs{};
}

__forceinline__ __device__ float thin_transmission_roughness_disney(
	const DisneySurface& surface) {

	(void)surface;
	return 0.0f;
}

__forceinline__ __device__ void anisotropic_params_disney(
	const DisneySurface& surface,
	float& ax,
	float& ay) {

	(void)surface;
	ax = 0.0f;
	ay = 0.0f;
}

__forceinline__ __device__ float3 tint_disney(
	const DisneySurface& surface) {

	(void)surface;
	return make_float3(0.0f);
}

__forceinline__ __device__ float3 eval_disney_diffuse_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wm,
	const float3& wi,
	const HitData& data,
	float& forwardPdf,
	float& reversePdf) {

	(void)surface;
	(void)wo;
	(void)wm;
	(void)wi;
	(void)data;
	forwardPdf = 0.0f;
	reversePdf = 0.0f;
	return make_float3(0.0f);
}

__forceinline__ __device__ float3 eval_disney_specular_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wm,
	const float3& wi,
	const HitData& data,
	float& forwardPdf,
	float& reversePdf) {

	(void)surface;
	(void)wo;
	(void)wm;
	(void)wi;
	(void)data;
	forwardPdf = 0.0f;
	reversePdf = 0.0f;
	return make_float3(0.0f);
}

__forceinline__ __device__ float3 eval_disney_clearcoat_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wm,
	const float3& wi,
	const HitData& data,
	float& forwardPdf,
	float& reversePdf) {

	(void)surface;
	(void)wo;
	(void)wm;
	(void)wi;
	(void)data;
	forwardPdf = 0.0f;
	reversePdf = 0.0f;
	return make_float3(0.0f);
}

__forceinline__ __device__ float3 eval_disney_spec_transmission_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wm,
	const float3& wi,
	const HitData& data,
	float& forwardPdf,
	float& reversePdf) {

	(void)surface;
	(void)wo;
	(void)wm;
	(void)wi;
	(void)data;
	forwardPdf = 0.0f;
	reversePdf = 0.0f;
	return make_float3(0.0f);
}

__forceinline__ __device__ float3 eval_disney_sheen_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wm,
	const float3& wi,
	const HitData& data) {

	(void)surface;
	(void)wo;
	(void)wm;
	(void)wi;
	(void)data;
	return make_float3(0.0f);
}

__forceinline__ __device__ BSDFSample sample_disney_diffuse_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	(void)surface;
	(void)wo;
	(void)data;
	(void)u;
	return BSDFSample{};
}

__forceinline__ __device__ BSDFSample sample_disney_specular_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	(void)surface;
	(void)wo;
	(void)data;
	(void)u;
	return BSDFSample{};
}

__forceinline__ __device__ BSDFSample sample_disney_clearcoat_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	(void)surface;
	(void)wo;
	(void)data;
	(void)u;
	return BSDFSample{};
}

__forceinline__ __device__ BSDFSample sample_disney_spec_transmission_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	(void)surface;
	(void)wo;
	(void)data;
	(void)u;
	return BSDFSample{};
}

__forceinline__ __device__ float pdf_disney(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	(void)wo;
	(void)wi;
	(void)mat;
	(void)data;
	return 0.0f;
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_disney(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	const DisneySurface surface = make_disney_surface(mat, data);
	const float3 wm = normalize(wo + wi);

	float forwardPdf = 0.0f;
	float reversePdf = 0.0f;
	float3 result = make_float3(0.0f);

	result += eval_disney_clearcoat_lobe(surface, wo, wm, wi, data, forwardPdf, reversePdf);
	result += eval_disney_diffuse_lobe(surface, wo, wm, wi, data, forwardPdf, reversePdf);
	result += eval_disney_spec_transmission_lobe(surface, wo, wm, wi, data, forwardPdf, reversePdf);
	result += eval_disney_specular_lobe(surface, wo, wm, wi, data, forwardPdf, reversePdf);
	result += eval_disney_sheen_lobe(surface, wo, wm, wi, data);

	return result;
}

__forceinline__ __device__ BSDFSample sample_disney(
	const float3& wo,
	const Material& mat,
	const HitData& data,
	const float3& u) {

	const DisneySurface surface = make_disney_surface(mat, data);
	const DisneyLobePdfs lobePdfs = calculate_disney_lobe_pdfs(surface);

	if (u.z < lobePdfs.specular)
		return sample_disney_specular_lobe(surface, wo, data, u);

	if (u.z < lobePdfs.specular + lobePdfs.clearcoat)
		return sample_disney_clearcoat_lobe(surface, wo, data, u);

	if (u.z < lobePdfs.specular + lobePdfs.clearcoat + lobePdfs.diffuse)
		return sample_disney_diffuse_lobe(surface, wo, data, u);

	return sample_disney_spec_transmission_lobe(surface, wo, data, u);
}
