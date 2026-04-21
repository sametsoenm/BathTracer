#pragma once

#include "rendersettings.h"
#include "camera/camera.h"
#include "device_types.h"
#include "util/texture.h"
#include <vector_functions.hpp>
#include <vector>
#include <string>
#include <memory>

class OptixScene {
public:
	explicit OptixScene(types::SceneType type);

	void addTriangle(
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, 
		const uint32_t matIdx,
		const glm::vec2& uv0 = glm::vec2(0.0f),
		const glm::vec2& uv1 = glm::vec2(0.0f),
		const glm::vec2& uv2 = glm::vec2(0.0f));

	void addTriangleLight(
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		const uint32_t matIdx);

	void addAreaLight(const glm::vec3& pos,
		const glm::vec3& u,
		const glm::vec3& v,
		const glm::vec3& emission);

	// returns material index
	uint32_t addLambertDiffuseMaterial(const glm::vec3& albedo);
	uint32_t addLambertDiffuseMaterial(const std::string& texPath);
	uint32_t addEmissiveDiffuseMaterial(const glm::vec3& emission);
	uint32_t addMirrorMaterial(const glm::vec3& reflectance);
	uint32_t addSmoothDielectricMaterial(const float eta);
	uint32_t addRoughDielectricMaterial(const float eta, const float alpha);
	uint32_t addSpecularMicrofacetMaterial(const glm::vec3& f0, const float alpha);
	uint32_t addSpecularMicrofacetMaterial(const std::string& f0Path, const std::string& alphaPath);

	const std::vector<GeometryData>& geometry() const { return _geometry; }
	const std::vector<Light>& lights() const { return _lights; }
	const std::vector<Material>& mats() const { return _mats; }
	const std::vector<Texture>& textures() const { return _textures; }
	const Camera& cam() const { return _cam; }
	Camera& cam() { return _cam; }
	const light::EnvironmentMap& environment() const { return _environment; }
	bool envmapEnabled() const { return _envmapEnabled; }
	void setEnvmapEnabled(bool b) { _envmapEnabled = b; }
	void loadEnvMap(std::string name);
	bool fogEnabled() const { return _fogEnabled; }
	void setFogEnabled(bool b) { _fogEnabled = b; }
	float fogDensity() const { return _fogDensity; }
	void setFogDensity(float mu_t) { _fogDensity = mu_t; }

private:
	void cornellScene();
	void sphereScene();
	void dragonScene();
	void glassScene();

	void load_obj(const std::string& path,
		const unsigned int defaultMaterialIdx,
		const float3& translation,
		const float3& scale);

private:
	Camera _cam;
	std::vector<GeometryData> _geometry;
	std::vector<Light> _lights;
	std::vector<Material> _mats;
	std::vector<Texture> _textures;
	light::EnvironmentMap _environment;

	bool _envmapEnabled = settings::ENV_MAP;
	bool _fogEnabled = settings::FOGGY;
	float _fogDensity = settings::FOG_DENSITY; // mu_t
};

struct SceneEntry {
	std::string name;
	std::unique_ptr<OptixScene> scene;
};