#include "optix_scene.h"

#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/gtx/norm.hpp>
#include <iostream>

void OptixScene::load_obj(const std::string& path,
    const unsigned int defaultMaterialIdx,
    const float3& translation,
    const float3& scale)
{
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    if (!reader.ParseFromFile(path, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader error: " << reader.Error() << std::endl;
        }
        return;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader warning: " << reader.Warning() << std::endl;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    const bool hasNormalsInFile = !attrib.normals.empty();
    const size_t vertexCount = attrib.vertices.size() / 3;

    std::vector<float3> accumulatedNormals(vertexCount, make_float3(0.0f, 0.0f, 0.0f));

    auto to_float3 = [](float x, float y, float z) -> float3 {
        return make_float3(x, y, z);
        };

    auto add = [](const float3& a, const float3& b) -> float3 {
        return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
        };

    auto sub = [](const float3& a, const float3& b) -> float3 {
        return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
        };

    auto mul = [](const float3& a, const float3& b) -> float3 {
        return make_float3(a.x * b.x, a.y * b.y, a.z * b.z);
        };

    auto cross = [](const float3& a, const float3& b) -> float3 {
        return make_float3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
        };

    auto dot = [](const float3& a, const float3& b) -> float {
        return a.x * b.x + a.y * b.y + a.z * b.z;
        };

    auto length2 = [&](const float3& v) -> float {
        return dot(v, v);
        };

    auto normalize_safe = [&](const float3& v, const float3& fallback) -> float3 {
        const float lenSq = length2(v);
        if (lenSq < 1e-20f) {
            return fallback;
        }
        const float invLen = 1.0f / std::sqrt(lenSq);
        return make_float3(v.x * invLen, v.y * invLen, v.z * invLen);
        };

    // -------------------------------------------------------------------------
    // Falls keine Vertex-Normals im OBJ vorhanden sind:
    // gemittelte Vertex-Normals aus Face-Normals berechnen
    // -------------------------------------------------------------------------
    if (!hasNormalsInFile) {
        for (const auto& shape : shapes) {
            size_t index_offset = 0;

            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                const int fv = shape.mesh.num_face_vertices[f];

                if (fv != 3) {
                    index_offset += fv;
                    continue;
                }

                const tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
                const tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
                const tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

                if (idx0.vertex_index < 0 || idx1.vertex_index < 0 || idx2.vertex_index < 0) {
                    index_offset += fv;
                    continue;
                }

                const int v0i = idx0.vertex_index;
                const int v1i = idx1.vertex_index;
                const int v2i = idx2.vertex_index;

                const float3 v0 = to_float3(
                    attrib.vertices[3 * v0i + 0],
                    attrib.vertices[3 * v0i + 1],
                    attrib.vertices[3 * v0i + 2]
                );

                const float3 v1 = to_float3(
                    attrib.vertices[3 * v1i + 0],
                    attrib.vertices[3 * v1i + 1],
                    attrib.vertices[3 * v1i + 2]
                );

                const float3 v2 = to_float3(
                    attrib.vertices[3 * v2i + 0],
                    attrib.vertices[3 * v2i + 1],
                    attrib.vertices[3 * v2i + 2]
                );

                const float3 e1 = sub(v1, v0);
                const float3 e2 = sub(v2, v0);
                const float3 faceCross = cross(e1, e2);

                if (length2(faceCross) < 1e-20f) {
                    index_offset += fv;
                    continue;
                }

                const float3 faceNormal = normalize_safe(faceCross, make_float3(0.0f, 1.0f, 0.0f));

                accumulatedNormals[v0i] = add(accumulatedNormals[v0i], faceNormal);
                accumulatedNormals[v1i] = add(accumulatedNormals[v1i], faceNormal);
                accumulatedNormals[v2i] = add(accumulatedNormals[v2i], faceNormal);

                index_offset += fv;
            }
        }

        for (float3& n : accumulatedNormals) {
            n = normalize_safe(n, make_float3(0.0f, 1.0f, 0.0f));
        }
    }

    // -------------------------------------------------------------------------
    // Dreiecke erzeugen
    // -------------------------------------------------------------------------
    for (const auto& shape : shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            const int fv = shape.mesh.num_face_vertices[f];

            if (fv != 3) {
                index_offset += fv;
                continue;
            }

            tinyobj::index_t indices[3];
            float3 positions[3];
            float3 normals[3];

            bool validTriangle = true;
            bool useVertexNormals = true;

            for (int v = 0; v < 3; ++v) {
                indices[v] = shape.mesh.indices[index_offset + v];
                const tinyobj::index_t idx = indices[v];

                if (idx.vertex_index < 0) {
                    validTriangle = false;
                    break;
                }

                const int vi = idx.vertex_index;

                float3 p = to_float3(
                    attrib.vertices[3 * vi + 0],
                    attrib.vertices[3 * vi + 1],
                    attrib.vertices[3 * vi + 2]
                );

                // Transform wie in der CPU-Version: p = p * scale + translation
                p = add(mul(p, scale), translation);
                positions[v] = p;

                if (hasNormalsInFile) {
                    if (idx.normal_index >= 0) {
                        const int ni = idx.normal_index;

                        float3 n = to_float3(
                            attrib.normals[3 * ni + 0],
                            attrib.normals[3 * ni + 1],
                            attrib.normals[3 * ni + 2]
                        );

                        // Bei non-uniform scale sollten Normalen eigentlich mit inverse-transpose
                        // transformiert werden. Die alte CPU-Version hat das nicht gemacht.
                        // Hier übernehmen wir das Verhalten 1:1.
                        normals[v] = normalize_safe(n, make_float3(0.0f, 1.0f, 0.0f));
                    }
                    else {
                        useVertexNormals = false;
                    }
                }
                else {
                    normals[v] = accumulatedNormals[vi];
                }
            }

            if (!validTriangle) {
                index_offset += fv;
                continue;
            }

            // Flat normal IMMER explizit berechnen
            const float3 e1 = sub(positions[1], positions[0]);
            const float3 e2 = sub(positions[2], positions[0]);
            const float3 flatCross = cross(e1, e2);

            if (length2(flatCross) < 1e-20f) {
                index_offset += fv;
                continue; // degeneriertes Dreieck überspringen
            }

            const float3 normalFlat = normalize_safe(flatCross, make_float3(0.0f, 1.0f, 0.0f));

            GeometryData geom{};
            geom.materialIndex = defaultMaterialIdx;

            geom.triangle.v0 = positions[0];
            geom.triangle.v1 = positions[1];
            geom.triangle.v2 = positions[2];
            geom.triangle.normalFlat = normalFlat;

            geom.triangle.hasVertexNormals = false;
            geom.triangle.n0 = make_float3(0.0f, 0.0f, 0.0f);
            geom.triangle.n1 = make_float3(0.0f, 0.0f, 0.0f);
            geom.triangle.n2 = make_float3(0.0f, 0.0f, 0.0f);

            if (hasNormalsInFile) {
                if (useVertexNormals) {
                    geom.triangle.hasVertexNormals = true;
                    geom.triangle.n0 = normals[0];
                    geom.triangle.n1 = normals[1];
                    geom.triangle.n2 = normals[2];
                }
            }
            else {
                geom.triangle.hasVertexNormals = true;
                geom.triangle.n0 = normals[0];
                geom.triangle.n1 = normals[1];
                geom.triangle.n2 = normals[2];
            }

            _geometry.push_back(geom); 

            index_offset += fv;
        }
    }

}