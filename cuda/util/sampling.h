#pragma once

#include "vec_math.h"

struct BSDFSample {
    float3 wi{ 0.0f };
    float pdf = 0.0f;
    float3 contrib{ 0.0f }; // returns (bsdf/pdf) * cos
    bool is_delta = false;
    float sign = 1.0; // 1 if reflected, -1 if transmitted
};

namespace sampling {

	__forceinline__ __device__ float3 sample_triangle(
        const float3& v0, 
        const float3& v1, 
        const float3& v2, 
        const float2& u) {

        float su = std::sqrt(u.x);

        float b0 = 1.0f - su;
        float b1 = u.y * su;
        float b2 = 1.0f - b0 - b1;

        return b0 * v0 + b1 * v1 + b2 * v2;
	}

    __forceinline__ __device__ float2 sample_uniform_disk(const float2& u) {
        const float r = sqrt(u.x);
        const float phi = 2.0f * M_PIf * u.y;

        const float x = r * cosf(phi);
        const float y = r * sinf(phi);

        return make_float2(x, y);
    }

    __forceinline__ __device__ float3 sample_cosine_hemisphere(const float2& u) {

        float3 dir;
        // Uniformly sample unit disk.
        const float r = sqrtf(u.x);
        const float phi = 2.0f * M_PIf * u.y;
        dir.x = r * cosf(phi);
        dir.y = r * sinf(phi);

        // Project to hemisphere.
        dir.z = sqrtf(fmaxf(0.0f, 1.0f - dir.x * dir.x - dir.y * dir.y));
        return dir;
    }

    __forceinline__ __device__ float3 sample_uniform_sphere(const float2& u) {
        const float z = 1.0f - 2.0f * u.x;
        float r = sqrtf(max(0.0f, 1.0f - z * z));
        const float phi = 2.0f * M_PIf * u.y;

        return make_float3(
            r * cosf(phi),
            r * sinf(phi),
            z
        );
    }

    static __forceinline__ __device__ float3 sampleGGXVNDFHemisphere(const float2& u, const float3& wo) {
        // orthonormal basis (with special case if cross product is 0)
        const float tmp = wo.x * wo.x + wo.y * wo.y;
        const float3 w1 = tmp > 0.0f
            ? make_float3(-wo.y, wo.x, 0.0f) * rsqrtf(tmp)
            : make_float3(1.0f, 0.0f, 0.0f);
        const float3 w2 = cross(wo, w1);
        // parameterization of the projected area
        const float r = sqrtf(u.x);
        const float phi = 2.f * static_cast<float>(M_PIf) * u.y;
        const float t1 = r * cosf(phi);
        float t2 = r * sinf(phi);
        const float s = (1.0f + wo.z) / 2.0f;
        t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
        const float ti = sqrtf(max(1.0f - t1 * t1 - t2 * t2, 0.0f));
        // reprojection onto hemisphere
        const float3 wm = t1 * w1 + t2 * w2 + ti * wo;
        // return hemispherical sample
        return wm;
    }

    __forceinline__ __device__ float3 sampleGGXVNDF(
        const float alpha, const float3& wo, const float2& u) {
        const float3 woStd = normalize(make_float3(wo.x * alpha, wo.y * alpha, wo.z));
        // sample the hemisphere
        const float3 wmStd = sampleGGXVNDFHemisphere(u, woStd);
        // warp back to the ellipsoid configuration
        const float3 wm = normalize(
            make_float3(wmStd.x * alpha, wmStd.y * alpha, max(0.f, wmStd.z))
        );
        // return final normal
        return wm;
    }

    __forceinline__ __device__ float eval_isotropic_phase_func() {
        return 0.25f / M_PIf;
    }

    __forceinline__ __device__ float pdf_uniform_triangle(
        const float3& e0,
        const float3& e1) {
        return 1.0f / (0.5f * length(cross(e0, e1)));
    }

    // solid angle measure
    __forceinline__ __device__ float pdf_uniform_triangle(
        const float3& e0,
        const float3& e1,
        const float3& normal,
        const float3& prevPos,
        const float3& triPos) {
        float3 wi = triPos - prevPos;
        const float d2 = dot(wi, wi);
        wi = normalize(wi);
        const float cosThetaJ = max(1e-8f, dot(normal, -wi));
        return pdf_uniform_triangle(e0, e1) * d2 / cosThetaJ;
    }

    __forceinline__ __device__ float pdf_cosine_hemisphere(float cos_theta) {
        return (cos_theta > 0.0f) ? cos_theta / M_PIf : 0.0f;
    }

    __forceinline__ __device__ float pdf_isotropic_phase_func() {
        return 0.25f / M_PIf;
    }

    __forceinline__ __device__ float power_heuristic(const float pdf1, const float pdf2) {
        const float a = pdf1 * pdf1;
        const float b = pdf2 * pdf2;
        return a / (a + b);
    }

}