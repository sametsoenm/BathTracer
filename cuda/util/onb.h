#pragma once

#include "vec_math.h"

struct Onb
{
    __forceinline__ __device__ Onb(const float3& __normal) {
        m_normal = __normal;

        if (fabs(m_normal.x) > fabs(m_normal.z))
        {
            m_binormal.x = -m_normal.y;
            m_binormal.y = m_normal.x;
            m_binormal.z = 0;
        }
        else
        {
            m_binormal.x = 0;
            m_binormal.y = -m_normal.z;
            m_binormal.z = m_normal.y;
        }

        m_binormal = normalize(m_binormal);
        m_tangent = cross(m_binormal, m_normal);
    }

    __forceinline__ __device__ void to_world_inplace(float3& p) const {
        p = p.x * m_tangent + p.y * m_binormal + p.z * m_normal;
    }

    __forceinline__ __device__ float3 to_world(const float3& p) const {
        return p.x * m_tangent + p.y * m_binormal + p.z * m_normal;
    }

    __forceinline__ __device__ void to_local_inplace(float3& v) const {
        v = make_float3(
            dot(v, m_tangent),
            dot(v, m_binormal),
            dot(v, m_normal)
        );
    }

    __forceinline__ __device__ float3 to_local(const float3& v) const {
        return make_float3(
            dot(v, m_tangent),
            dot(v, m_binormal),
            dot(v, m_normal)
        );
    }

    float3 m_tangent;
    float3 m_binormal;
    float3 m_normal;
};
