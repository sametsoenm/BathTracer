#pragma once

#include "device_types.h"
#include "material_funcs.h"
#include "util/vec_math.h"
#include "util/onb.h"
#include "util/sampling.h"

struct DisneySurface {
	float3 baseColor;
	float3 transmittanceColor;

	float metallic;
	float roughness;
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
	surface.baseColor = mat.color;
	surface.transmittanceColor = mat.transmittanceColor;
	surface.metallic = mat.metallic;
	surface.roughness = mat.roughness;
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

	const float metallicBRDF = surface.metallic;
	const float specularBSDF = (1.0f - surface.metallic) * surface.specTrans;
	const float dielectricBRDF = (1.0f - surface.specTrans) * (1.0f - surface.metallic);

	const float specularWeight = metallicBRDF + dielectricBRDF;
	const float transmissionWeight = specularBSDF;
	const float diffuseWeight = dielectricBRDF;
	const float clearcoatWeight = clamp(surface.clearcoat, 0.0f, 1.0f);
	const float sum = specularWeight + transmissionWeight + diffuseWeight + clearcoatWeight;

	if (sum <= 0.0f)
		return DisneyLobePdfs{};

	const float invSum = 1.0f / sum;
	DisneyLobePdfs pdfs{};
	pdfs.specular = specularWeight * invSum;
	pdfs.diffuse = diffuseWeight * invSum;
	pdfs.clearcoat = clearcoatWeight * invSum;
	pdfs.specTransmission = transmissionWeight * invSum;
	return pdfs;
}

__forceinline__ __device__ float thin_transmission_roughness_disney(
	const DisneySurface& surface) {

	return clamp((0.65f * surface.eta - 0.35f) * surface.roughness, 0.0f, 1.0f);
}

__forceinline__ __device__ void anisotropic_params_disney(
	const DisneySurface& surface,
	float& ax,
	float& ay) {

	material::anisotropic_params_burley(surface.roughness, surface.anisotropic, ax, ay);
}

__forceinline__ __device__ float3 tint_disney(
	const DisneySurface& surface) {

	const float luminance =
        0.3f * surface.baseColor.x +
        0.6f * surface.baseColor.y +
        0.1f * surface.baseColor.z;
	return luminance > 0.0f ? surface.baseColor / luminance : make_float3(1.0f);
}

__forceinline__ __device__ float3 disney_fresnel(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi) {

	const float wo_dot_wh = fabsf(dot(wh, wo));
	const float wi_dot_wh = fabsf(dot(wi, wh));
	const float relativeIor = 1.0f / surface.eta;
	const float3 tint = tint_disney(surface);

	float3 R0 =
		material::schlick_R0_from_relative_ior(relativeIor) *
		lerp(make_float3(1.0f), tint, surface.specularTint);
	R0 = lerp(R0, surface.baseColor, surface.metallic);

	// const float dielectricFresnel =
	//  	material::fresnel_dielectric(wo_dot_wh, 1.0f, surface.eta);
	const float3 dielectricFresnel =
		material::fresnel_schlick(R0, wo_dot_wh);
	const float3 metallicFresnel =
		material::fresnel_schlick(R0, wi_dot_wh);

	return lerp(dielectricFresnel, metallicFresnel, surface.metallic);
}

__forceinline__ __device__ float3 eval_disney_diffuse_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi,
	const HitData& data) {

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);
	const float3 wh_local = onb.to_local(wh);

	if (wo_local.z <= 0.0f || wi_local.z <= 0.0f)
		return make_float3(0.0f);

	const float cosThetaI = wi_local.z;
	const float cosThetaO = wo_local.z;
	const float cosThetaD = clamp(dot(wi_local, wh_local), 0.0f, 1.0f);

	const float FL = powf(1.0f - cosThetaI, 5.0f);
	const float FV = powf(1.0f - cosThetaO, 5.0f);

	float hanrahanKrueger = 0.0f;
	if (surface.thin && surface.flatness > 0.0f) {
		const float roughness2 = surface.roughness * surface.roughness;
		const float FSS90 = cosThetaD * cosThetaD * roughness2;
		const float FSS =
			(1.0f + FL * (FSS90 - 1.0f)) *
			(1.0f + FV * (FSS90 - 1.0f));
		hanrahanKrueger =
			1.25f * (FSS * (1.0f / (cosThetaI + cosThetaO) - 0.5f) + 0.5f);
	}

	const float RR = 2.0f * surface.roughness * cosThetaD * cosThetaD;
	const float lambert = 1.0f;
	const float retroReflection = RR * (FL + FV + FL * FV * (RR - 1.0f));
	const float subsurfaceApprox =
		lambert + (surface.thin ? surface.flatness : 0.0f) * (hanrahanKrueger - lambert);
	const float diffuse =
		M_1_PIf *
		(retroReflection + subsurfaceApprox * (1.0f - 0.5f * FL) * (1.0f - 0.5f * FV));

	const float V = dot(data.geo_normal, wi) <= 0.0f ? 0.0f : 1.0f;
	return V * surface.baseColor * diffuse * cosThetaI;
}

__forceinline__ __device__ float3 eval_disney_specular_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi,
	const HitData& data) {

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);
	const float3 wh_local = onb.to_local(wh);

	if (wo_local.z <= 0.0f || wi_local.z <= 0.0f || wh_local.z <= 0.0f)
		return make_float3(0.0f);

	const float wo_dot_wh = dot(wo_local, wh_local);
	const float wi_dot_wh = dot(wi_local, wh_local);
	const float wo_dot_wh_abs = fabsf(wo_dot_wh);
	const float wi_dot_wh_abs = fabsf(wi_dot_wh);

	if (wo_dot_wh_abs <= 0.0f || wi_dot_wh_abs <= 0.0f)
		return make_float3(0.0f);

	float ax = 0.0f;
	float ay = 0.0f;
	anisotropic_params_disney(surface, ax, ay);

	const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
	const float G = material::G_smith_anisotropic(ax, ay, wi_local, wo_local, wh_local);

	const float3 F = disney_fresnel(surface, wo_local, wh_local, wi_local);

	const float V = dot(data.geo_normal, wi) <= 0.0f ? 0.0f : 1.0f;
	return clamp(V * F * D * G / (4.0f * wo_local.z), 0.0f, 1e36f);
}

__forceinline__ __device__ float eval_disney_clearcoat_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi,
	const HitData& data) {

	if (surface.clearcoat <= 0.0f)
		return 0.0f;

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);
	const float3 wh_local = onb.to_local(wh);

	if (wo_local.z <= 0.0f || wi_local.z <= 0.0f || wh_local.z <= 0.0f)
		return 0.0f;

	const float wo_dot_wh = dot(wo_local, wh_local);
	const float wi_dot_wh = dot(wi_local, wh_local);
	const float wo_dot_wh_abs = fabsf(wo_dot_wh);
	const float wi_dot_wh_abs = fabsf(wi_dot_wh);

	if (wo_dot_wh_abs <= 0.0f || wi_dot_wh_abs <= 0.0f)
		return 0.0f;

	const float alpha = 0.1f + surface.clearcoatGloss * (0.001f - 0.1f); // maybe change
	const float D = material::GTR1_burley(alpha, wh_local.z);
	const float F = material::fresnel_schlick(make_float3(0.04f), wi_dot_wh).x;
	const float G = material::G_smith(0.25f, wi_local, wo_local, wh_local);

	const float brdf = 0.25f * surface.clearcoat * D * F * G;
	return brdf * wi_local.z;
}

__forceinline__ __device__ float3 eval_disney_spec_transmission_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi,
	const HitData& data) {

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);
	const float3 wh_local = onb.to_local(wh);

	const float wi_dot_n_abs = fabsf(wi_local.z);
	const float wo_dot_n_abs = fabsf(wo_local.z);
	const float wi_dot_wh = dot(wi_local, wh_local);
	const float wo_dot_wh = dot(wo_local, wh_local);
	const float wi_dot_wh_abs = fabsf(wi_dot_wh);
	const float wo_dot_wh_abs = fabsf(wo_dot_wh);

	if (wi_dot_n_abs <= 0.0f || wo_dot_n_abs <= 0.0f || wi_dot_wh_abs <= 0.0f || wo_dot_wh_abs <= 0.0f)
		return make_float3(0.0f);

	const float roughness = surface.thin
		? thin_transmission_roughness_disney(surface)
		: surface.roughness;

	float ax = 0.0f;
	float ay = 0.0f;
	material::anisotropic_params_burley(roughness, surface.anisotropic, ax, ay);

	const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
	const float G = material::G_smith_anisotropic_abs(ax, ay, wi_local, wo_local, wh_local);
	const float F = material::fresnel_dielectric(wo_dot_wh, 1.0f, surface.eta);

	const float relativeIor = 1.0f / surface.eta;
	const float n2 = relativeIor * relativeIor;
	const float denom = wi_dot_wh + relativeIor * wo_dot_wh;
	const float reverseDenom = wo_dot_wh + relativeIor * wi_dot_wh;

	if (denom == 0.0f || reverseDenom == 0.0f)
		return make_float3(0.0f);

	const float3 color = surface.thin
		? make_float3(
			sqrtf(surface.baseColor.x),
			sqrtf(surface.baseColor.y),
			sqrtf(surface.baseColor.z))
		: surface.baseColor;

	const float c = (wi_dot_wh_abs * wo_dot_wh_abs) / (wi_dot_n_abs * wo_dot_n_abs);
	const float t = n2 / (denom * denom);
	const float3 transmission = color * c * t * (1.0f - F) * G * D;

	const float V = dot(data.geo_normal, wi) == 0.0f ? 0.0f : 1.0f;
	return V * transmission * wi_dot_n_abs;
}

__forceinline__ __device__ float3 eval_disney_sheen_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const float3& wh,
	const float3& wi,
	const HitData& data) {

	const float cos = clamp(dot(wh, wi), 0.0f, 1.0f);
    const float3 tint = tint_disney(surface);
	return /*10.f * */ surface.sheen * lerp(make_float3(1.0f), tint, surface.sheenTint) * powf((1.0f - cos), 5.0f);
}

__forceinline__ __device__ BSDFSample sample_disney_diffuse_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	BSDFSample s{};

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	if (wo_local.z == 0.0f)
		return s;

	const float wo_sign = wo_local.z < 0.0f ? -1.0f : 1.0f;
	float3 wi_local = wo_sign * sampling::sample_cosine_hemisphere(make_float2(u));
	const float3 wm_local = normalize(wi_local + wo_local);

	const float diffTrans = clamp(surface.diffTrans, 0.0f, 1.0f);
	float branchPdf = 1.0f - diffTrans;
	float branchWeight = branchPdf;
	bool diffuseTransmission = false;
	float3 color = surface.baseColor;

	if (u.z <= diffTrans) {
		wi_local = -wi_local;
		branchPdf = diffTrans;
		branchWeight = diffTrans;
		diffuseTransmission = true;
		s.sign = -1.0f;

		if (surface.thin) {
			color = make_float3(
				sqrtf(color.x),
				sqrtf(color.y),
				sqrtf(color.z));
		}
	}

	const float wi_dot_n_abs = fabsf(wi_local.z);
	const float wo_dot_n_abs = fabsf(wo_local.z);
	const float pdf = sampling::pdf_cosine_hemisphere(wi_dot_n_abs) * branchPdf;
	if (pdf <= 0.0f) {
		return s;
	}

	const float cosThetaD = fabsf(dot(wi_local, wm_local));
	const float FL = powf(1.0f - wi_dot_n_abs, 5.0f);
	const float FV = powf(1.0f - wo_dot_n_abs, 5.0f);

	float hanrahanKrueger = 0.0f;
	if (surface.thin && surface.flatness > 0.0f) {
		const float roughness2 = surface.roughness * surface.roughness;
		const float FSS90 = cosThetaD * cosThetaD * roughness2;
		const float FSS =
			(1.0f + FL * (FSS90 - 1.0f)) *
			(1.0f + FV * (FSS90 - 1.0f));
		hanrahanKrueger =
			1.25f * (FSS * (1.0f / (wi_dot_n_abs + wo_dot_n_abs) - 0.5f) + 0.5f);
	}

	const float RR = 2.0f * surface.roughness * cosThetaD * cosThetaD;
	const float retroReflection = RR * (FL + FV + FL * FV * (RR - 1.0f));
	const float subsurfaceApprox =
		1.0f + (surface.thin ? surface.flatness : 0.0f) * (hanrahanKrueger - 1.0f);
	const float diffuse =
		M_1_PIf *
		(retroReflection + subsurfaceApprox * (1.0f - 0.5f * FL) * (1.0f - 0.5f * FV));

	const float3 wi = normalize(onb.to_world(wi_local));
	const float3 wh = normalize(onb.to_world(wm_local));
	const float diffuseWeight = (1.0f - surface.metallic) * (1.0f - surface.specTrans);
	const float3 sheen = diffuseTransmission
		? make_float3(0.0f)
		: eval_disney_sheen_lobe(surface, wo, wh, wi, data);
	const float3 bsdfCos = diffuseWeight * branchWeight * (color * diffuse + sheen) * wi_dot_n_abs;

	s.wi = wi;
	s.pdf = pdf;
	s.contrib = bsdfCos / pdf;
	return s;
}

__forceinline__ __device__ BSDFSample sample_disney_specular_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	BSDFSample s{};

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);

	if (wo_local.z <= 0.0f)
		return s;

	float ax = 0.0f;
	float ay = 0.0f;
	anisotropic_params_disney(surface, ax, ay);

	const float3 wh_local = sampling::sampleGGXVNDFAnisotropic(ax, ay, wo_local, make_float2(u));
	const float wo_dot_wh = dot(wo_local, wh_local);

	if (wh_local.z <= 0.0f || wo_dot_wh <= 0.0f)
		return s;

	const float3 wi_local = reflect(-wo_local, wh_local);
	if (wi_local.z <= 0.0f)
		return s;

	const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
	const float G1 = material::G1_smith_anisotropic(ax, ay, wo_local, wh_local);
	const float pdf = G1 * wo_dot_wh * D / (4.0f * wo_dot_wh * wo_local.z);

	if (pdf <= 0.0f)
		return s;

	const float3 wi = normalize(onb.to_world(wi_local));
	const float3 wh = normalize(onb.to_world(wh_local));

	const float3 bsdfCos = eval_disney_specular_lobe(surface, wo, wh, wi, data);

	s.wi = wi;
	s.pdf = pdf;
	s.contrib = bsdfCos / pdf;
	return s;
}

__forceinline__ __device__ BSDFSample sample_disney_clearcoat_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	BSDFSample s{};

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	if (wo_local.z <= 0.0f)
		return s;

	const float alpha = 0.1f + surface.clearcoatGloss * (0.001f - 0.1f);
	const float alpha2 = alpha * alpha;
	const float cosTheta = sqrtf(fmaxf(
		0.0f,
		(1.0f - powf(alpha2, 1.0f - u.x)) / (1.0f - alpha2)));
	const float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - cosTheta * cosTheta));
	const float phi = 2.0f * M_PIf * u.y;

	float3 wh_local = make_float3(
		sinTheta * cosf(phi),
		sinTheta * sinf(phi),
		cosTheta);

	if (dot(wh_local, wo_local) < 0.0f)
		wh_local = -wh_local;

	const float wo_dot_wh = dot(wo_local, wh_local);
	if (wh_local.z <= 0.0f || wo_dot_wh <= 0.0f)
		return s;

	const float3 wi_local = reflect(-wo_local, wh_local);
	if (wi_local.z <= 0.0f || dot(wi_local, wo_local) <= 0.0f)
		return s;

	const float D = material::GTR1_burley(alpha, wh_local.z);
	const float pdf = D * wh_local.z / (4.0f * wo_dot_wh);
	if (pdf <= 0.0f)
		return s;

	const float3 wi = normalize(onb.to_world(wi_local));
	const float3 wh = normalize(onb.to_world(wh_local));

	const float bsdfCos = eval_disney_clearcoat_lobe(surface, wo, wh, wi, data);

	s.wi = wi;
	s.pdf = pdf;
	s.contrib = make_float3(bsdfCos / pdf);
	return s;
}

__forceinline__ __device__ BSDFSample sample_disney_spec_transmission_lobe(
	const DisneySurface& surface,
	const float3& wo,
	const HitData& data,
	const float3& u) {

	BSDFSample s{};

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);

	if (wo_local.z == 0.0f)
		return s;

	const float roughness = surface.thin
		? thin_transmission_roughness_disney(surface)
		: surface.roughness;

	float ax = 0.0f;
	float ay = 0.0f;
	material::anisotropic_params_burley(roughness, surface.anisotropic, ax, ay);

	float3 wh_local = sampling::sampleGGXVNDFAnisotropic(ax, ay, wo_local, make_float2(u));
	float wo_dot_wh = dot(wo_local, wh_local);

	if (wo_dot_wh < 0.0f) {
		wh_local = -wh_local;
		wo_dot_wh = -wo_dot_wh;
	}

	if (wh_local.z == 0.0f || wo_dot_wh <= 0.0f)
		return s;

	const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
	const float G1v = material::G1_smith_anisotropic_abs(ax, ay, wo_local, wh_local);
	const float vndfPdf = G1v * wo_dot_wh * D / fabsf(wo_local.z);
	if (vndfPdf <= 0.0f)
		return s;

	const float F = material::fresnel_dielectric(wo_dot_wh, 1.0f, surface.eta);
	const float transWeight = (1.0f - surface.metallic) * surface.specTrans;
	const bool reflectEvent = u.z <= F;
	float3 wi_local = make_float3(0.0f);
	float pdf = 0.0f;
	float3 color = surface.baseColor;

	if (reflectEvent) {
		wi_local = reflect(-wo_local, wh_local);
		const float jacobian = 1.0f / (4.0f * wo_dot_wh);
		pdf = vndfPdf * F * jacobian;
	}
	else {
		if (surface.thin) {
			wi_local = reflect(-wo_local, wh_local);
			wi_local.z = -wi_local.z;
			s.sign = -1.0f;
			color = make_float3(
				sqrtf(surface.baseColor.x),
				sqrtf(surface.baseColor.y),
				sqrtf(surface.baseColor.z));
		}
		else {
			const float etaI = wo_local.z > 0.0f ? 1.0f : surface.eta;
			const float etaT = wo_local.z > 0.0f ? surface.eta : 1.0f;
			wi_local = material::refract(-wo_local, wh_local, etaI, etaT);

			if (dot(wi_local, wi_local) == 0.0f) {
				wi_local = reflect(-wo_local, wh_local);
			}
			else {
				s.sign = -1.0f;
			}
		}

		wi_local = normalize(wi_local);
		const float wi_dot_wh = fabsf(dot(wi_local, wh_local));
		const float relativeIor = 1.0f / surface.eta;
		const float denom = wi_dot_wh + relativeIor * wo_dot_wh;

		if (wi_dot_wh <= 0.0f || denom == 0.0f)
			return s;

		const float jacobian = wi_dot_wh / (denom * denom);
		pdf = vndfPdf * (1.0f - F) * jacobian;
	}

	if (wi_local.z == 0.0f || pdf <= 0.0f)
		return s;

	const float3 wi = normalize(onb.to_world(wi_local));

	s.wi = wi;
	s.pdf = pdf;
	s.contrib = transWeight * G1v * color;
	return s;
}

__forceinline__ __device__ float pdf_disney(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	const DisneySurface surface = make_disney_surface(mat, data);
	const DisneyLobePdfs lobePdfs = calculate_disney_lobe_pdfs(surface);

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);

	if (wo_local.z == 0.0f || wi_local.z == 0.0f)
		return 0.0f;

	const bool sameHemisphere = wo_local.z * wi_local.z > 0.0f;
	const float diffTrans = clamp(surface.diffTrans, 0.0f, 1.0f);
	const float wi_dot_n_abs = fabsf(wi_local.z);
	float pdf = 0.0f;

	// diffuse lobe cosine sampling, with optional diffuse transmission branch
	if (lobePdfs.diffuse > 0.0f) {
		const float diffuseBranchPdf = sameHemisphere
			? 1.0f - diffTrans
			: diffTrans;
		pdf += lobePdfs.diffuse *
			sampling::pdf_cosine_hemisphere(wi_dot_n_abs) *
			diffuseBranchPdf;
	}

	if (sameHemisphere) {
		const float3 wh_local = normalize(wo_local + wi_local);
		const float wo_dot_wh = dot(wo_local, wh_local);
		const float wi_dot_wh = dot(wi_local, wh_local);

		if (wh_local.z > 0.0f && wo_dot_wh > 0.0f && wi_dot_wh > 0.0f) {
			float ax = 0.0f;
			float ay = 0.0f;
			anisotropic_params_disney(surface, ax, ay);

			const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
			const float G1v = material::G1_smith_anisotropic(ax, ay, wo_local, wh_local);
			const float vndfPdf = G1v * wo_dot_wh * D / wo_local.z;

			// specular reflection lobe
			if (lobePdfs.specular > 0.0f) {
				pdf += lobePdfs.specular * vndfPdf / (4.0f * wo_dot_wh);
			}

			// specular transmission lobe can also sample a reflected fresnel event
			if (lobePdfs.specTransmission > 0.0f) {
				const float roughness = surface.thin
					? thin_transmission_roughness_disney(surface)
					: surface.roughness;
				float tax = 0.0f;
				float tay = 0.0f;
				material::anisotropic_params_burley(roughness, surface.anisotropic, tax, tay);

				const float tD = material::NDF_GTR2_anisotropic(tax, tay, wh_local);
				const float tG1v = material::G1_smith_anisotropic_abs(tax, tay, wo_local, wh_local);
				const float tVndfPdf = tG1v * wo_dot_wh * tD / fabsf(wo_local.z);
				const float F = material::fresnel_dielectric(wo_dot_wh, 1.0f, surface.eta);
				pdf += lobePdfs.specTransmission * tVndfPdf * F / (4.0f * wo_dot_wh);
			}

			// clearcoat lobe
			if (lobePdfs.clearcoat > 0.0f && surface.clearcoat > 0.0f) {
				const float alpha = 0.1f + surface.clearcoatGloss * (0.001f - 0.1f);
				const float cD = material::GTR1_burley(alpha, wh_local.z);
				pdf += lobePdfs.clearcoat * cD * wh_local.z / (4.0f * wo_dot_wh);
			}
		}
	}
	else if (lobePdfs.specTransmission > 0.0f) {
		const float roughness = surface.thin
			? thin_transmission_roughness_disney(surface)
			: surface.roughness;
		float ax = 0.0f;
		float ay = 0.0f;
		material::anisotropic_params_burley(roughness, surface.anisotropic, ax, ay);

		float3 wh_local = make_float3(0.0f);
		float jacobian = 0.0f;

		if (surface.thin) {
			const float3 reflected_wi_local =
				make_float3(wi_local.x, wi_local.y, -wi_local.z);
			wh_local = normalize(wo_local + reflected_wi_local);
			const float wo_dot_wh = fabsf(dot(wo_local, wh_local));
			if (wo_dot_wh > 0.0f)
				jacobian = 1.0f / (4.0f * wo_dot_wh);
		}
		else {
			const float etaI = wo_local.z > 0.0f ? 1.0f : surface.eta;
			const float etaT = wo_local.z > 0.0f ? surface.eta : 1.0f;
			const float relativeIor = 1.0f / surface.eta;

			wh_local = normalize(etaT * wi_local + etaI * wo_local);
			if (dot(wo_local, wh_local) < 0.0f)
				wh_local = -wh_local;

			const float wo_dot_wh = fabsf(dot(wo_local, wh_local));
			const float wi_dot_wh = fabsf(dot(wi_local, wh_local));
			const float denom = wi_dot_wh + relativeIor * wo_dot_wh;
			if (wi_dot_wh > 0.0f && denom != 0.0f)
				jacobian = wi_dot_wh / (denom * denom);
		}

		const float wo_dot_wh = fabsf(dot(wo_local, wh_local));
		if (wh_local.z > 0.0f && wo_dot_wh > 0.0f && jacobian > 0.0f) {
			const float D = material::NDF_GTR2_anisotropic(ax, ay, wh_local);
			const float G1v = material::G1_smith_anisotropic_abs(ax, ay, wo_local, wh_local);
			const float vndfPdf = G1v * wo_dot_wh * D / fabsf(wo_local.z);
			const float F = material::fresnel_dielectric(wo_dot_wh, 1.0f, surface.eta);
			pdf += lobePdfs.specTransmission * vndfPdf * (1.0f - F) * jacobian;
		}
	}

	return pdf;
}

// evaluates bsdf * cosTheta
__forceinline__ __device__ float3 eval_disney(
	const float3& wo,
	const float3& wi,
	const Material& mat,
	const HitData& data) {

	const DisneySurface surface = make_disney_surface(mat, data);
	const float3 wh = normalize(wo + wi);

	Onb onb(data.shading_normal);
	const float3 wo_local = onb.to_local(wo);
	const float3 wi_local = onb.to_local(wi);
	const float wo_dot_n = wo_local.z;
	const float wi_dot_n = wi_local.z;
	const bool upperHemisphere = wo_dot_n > 0.0f && wi_dot_n > 0.0f;

	float3 result = make_float3(0.0f);

	if (upperHemisphere && surface.clearcoat > 0.0f) {
		const float clearcoat =
			eval_disney_clearcoat_lobe(surface, wo, wh, wi, data);
		result += make_float3(clearcoat);
	}

	const float diffuseWeight = (1.0f - surface.metallic) * (1.0f - surface.specTrans);
	if (diffuseWeight > 0.0f) {
		const float diffuseReflectWeight = 1.0f - clamp(surface.diffTrans, 0.0f, 1.0f);
		result += diffuseWeight * diffuseReflectWeight *
			eval_disney_diffuse_lobe(surface, wo, wh, wi, data);

		if (upperHemisphere) {
			const float3 sheen = eval_disney_sheen_lobe(surface, wo, wh, wi, data);
			const float V = dot(data.geo_normal, wi) <= 0.0f ? 0.0f : 1.0f;
			result += V * diffuseWeight * diffuseReflectWeight * sheen * wi_dot_n;
		}
	}

	const float transWeight = (1.0f - surface.metallic) * surface.specTrans;
	if (transWeight > 0.0f) {
		result += transWeight * eval_disney_spec_transmission_lobe(surface, wo, wh, wi, data);
	}

	if (upperHemisphere) {
		result += eval_disney_specular_lobe(surface, wo, wh, wi, data);
	}

	return result;
}

__forceinline__ __device__ BSDFSample sample_disney(
	const float3& wo,
	const Material& mat,
	const HitData& data,
	const float3& u) {

	const DisneySurface surface = make_disney_surface(mat, data);
	const DisneyLobePdfs lobePdfs = calculate_disney_lobe_pdfs(surface);

	BSDFSample s{};
	float pLobe = 0.0f;
	float lobeStart = 0.0f;

	if (u.z < lobePdfs.specular) {
		const float3 lobeU = make_float3(u.x, u.y, u.z / lobePdfs.specular);
		s = sample_disney_specular_lobe(surface, wo, data, lobeU);
		pLobe = lobePdfs.specular;
	}
	else if (u.z < lobePdfs.specular + lobePdfs.clearcoat) {
		lobeStart = lobePdfs.specular;
		const float3 lobeU = make_float3(u.x, u.y, (u.z - lobeStart) / lobePdfs.clearcoat);
		s = sample_disney_clearcoat_lobe(surface, wo, data, lobeU);
		pLobe = lobePdfs.clearcoat;
	}
	else if (u.z < lobePdfs.specular + lobePdfs.clearcoat + lobePdfs.diffuse) {
		lobeStart = lobePdfs.specular + lobePdfs.clearcoat;
		const float3 lobeU = make_float3(u.x, u.y, (u.z - lobeStart) / lobePdfs.diffuse);
		s = sample_disney_diffuse_lobe(surface, wo, data, lobeU);
		pLobe = lobePdfs.diffuse;
	}
	else {
		if (lobePdfs.specTransmission <= 0.0f)
			return s;

		lobeStart = lobePdfs.specular + lobePdfs.clearcoat + lobePdfs.diffuse;
		const float3 lobeU = make_float3(u.x, u.y, (u.z - lobeStart) / lobePdfs.specTransmission);
		s = sample_disney_spec_transmission_lobe(surface, wo, data, lobeU);
		pLobe = lobePdfs.specTransmission;
	}

	if (pLobe > 0.0f) {
		s.contrib /= pLobe;
		s.pdf *= pLobe;
	}

	return s;
}
