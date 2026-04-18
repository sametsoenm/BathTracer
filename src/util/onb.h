#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

struct Onb
{
    Onb(const glm::vec3& __normal) {
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

        m_binormal = glm::normalize(m_binormal);
        m_tangent = glm::cross(m_binormal, m_normal);
    }

    void to_world_inplace(glm::vec3& p) const {
        p = p.x * m_tangent + p.y * m_binormal + p.z * m_normal;
    }

    glm::vec3 to_world(const glm::vec3& p) const {
        return p.x * m_tangent + p.y * m_binormal + p.z * m_normal;
    }

    void to_local_inplace(glm::vec3& v) const {
        v = glm::vec3(
            glm::dot(v, m_tangent),
            glm::dot(v, m_binormal),
            glm::dot(v, m_normal)
        );
    }

    glm::vec3 to_local(const glm::vec3& v) const {
        return glm::vec3(
            glm::dot(v, m_tangent),
            glm::dot(v, m_binormal),
            glm::dot(v, m_normal)
        );
    }

    glm::vec3 m_tangent;
    glm::vec3 m_binormal;
    glm::vec3 m_normal;
};
