#include "optix_scene.h"

#include <vector_functions.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include <iostream>

OptixScene::OptixScene(types::SceneType type) {
	switch (type) {
	case types::SceneType::CORNELL_SCENE:
		cornellScene();
		break;
	case types::SceneType::SPHERES_SCENE:
		sphereScene();
		break;
	case types::SceneType::DRAGON_SCENE:
		dragonScene();
		break;
	case types::SceneType::GLASS_SCENE:
		glassScene();
		break;
	case types::SceneType::EMPTY_SCENE:
		break;
	default:
		cornellScene();
		break;
	}

	_environment = light::envMaps[0];
}

static GeometryData createTriangle(
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, 
	const uint32_t matIdx,
	const glm::vec2& uv0 = glm::vec2(0.0f), 
	const glm::vec2& uv1 = glm::vec2(0.0f), 
	const glm::vec2& uv2 = glm::vec2(0.0f)) {

	GeometryData t{};
	const glm::vec3 e1 = v1 - v0;
	const glm::vec3 e2 = v2 - v0;
	const glm::vec3 n = glm::normalize(glm::cross(e1, e2));
	t.triangle = {
		make_float3(v0.x, v0.y, v0.z),
		make_float3(v1.x, v1.y, v1.z),
		make_float3(v2.x, v2.y, v2.z),
		make_float3(n.x, n.y, n.z),
	};
	t.triangle.uv0 = make_float2(uv0.x, uv0.y);
	t.triangle.uv1 = make_float2(uv1.x, uv1.y);
	t.triangle.uv2 = make_float2(uv2.x, uv2.y);
	t.materialIndex = matIdx;
	return t;
}

static Light createTriangleLight(
	const glm::vec3& v0, 
	const glm::vec3& v1, 
	const glm::vec3& v2, 
	const uint32_t matIdx) {
	auto t = createTriangle(v0, v1, v2, matIdx);
	Light l{};
	l.type = LightType::TRIANGLE_LIGHT;
	l.geoLight = t;
	return l;
}

static Light createAreaLight(
	const glm::vec3& pos, 
	const glm::vec3& u, 
	const glm::vec3& v, 
	const glm::vec3& emission) {
	Light l{};
	l.type = LightType::AREA_LIGHT;
	l.absLight.pos = make_float3(pos.x, pos.y, pos.z);
	l.absLight.u = make_float3(u.x, u.y, u.z);
	l.absLight.v = make_float3(v.x, v.y, v.z);
	l.absLight.emission = make_float3(emission.x, emission.y, emission.z);
	return l;
}

void OptixScene::addTriangle(
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, 
	const uint32_t matIdx,
	const glm::vec2& uv0,
	const glm::vec2& uv1,
	const glm::vec2& uv2) {
	_geometry.push_back(createTriangle(v0, v1, v2, matIdx, uv0, uv1, uv2));
}

void OptixScene::addTriangleLight(
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const uint32_t matIdx) {
	Light l = createTriangleLight(v0, v1, v2, matIdx);
	_geometry.push_back(l.geoLight);
	_lights.push_back(l);
}

void OptixScene::addAreaLight(const glm::vec3& pos,
	const glm::vec3& u,
	const glm::vec3& v,
	const glm::vec3& emission) {
	_lights.push_back(createAreaLight(pos, u, v, emission));
}

uint32_t OptixScene::addLambertDiffuseMaterial(const glm::vec3& albedo) {
	Texture tex(albedo);
	_textures.push_back(tex);

	Material mat{};
	mat.type = MaterialType::LAMBERT_DIFFUSE;
	mat.color = make_float3(albedo.x, albedo.y, albedo.z);
	mat.colorTexIdx = _textures.size() - 1;
	_mats.push_back(mat);
	return _mats.size() - 1;
}
uint32_t OptixScene::addLambertDiffuseMaterial(const std::string& texPath) {
	Texture tex(texPath, true);
	_textures.push_back(tex);

	Material mat{};
	mat.type = MaterialType::LAMBERT_DIFFUSE;
	mat.colorTexIdx = _textures.size() - 1;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

uint32_t OptixScene::addEmissiveDiffuseMaterial(const glm::vec3& emission) {
	Material mat{};
	mat.type = MaterialType::EMISSIVE_DIFFUSE;
	mat.emission = make_float3(emission.x, emission.y, emission.z);
	_mats.push_back(mat);
	return _mats.size() - 1;
}


uint32_t OptixScene::addMirrorMaterial(const glm::vec3& reflectance) {
	Material mat{};
	mat.type = MaterialType::MIRROR;
	mat.reflectance = make_float3(reflectance.x, reflectance.y, reflectance.z);
	mat.isDelta = true;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

uint32_t OptixScene::addSmoothDielectricMaterial(const float eta) {
	Material mat{};
	mat.type = MaterialType::SMOOTH_DIELECTRIC;
	mat.eta = eta;
	mat.isDelta = true;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

uint32_t OptixScene::addRoughDielectricMaterial(const float eta, const float alpha) {
	Material mat{};
	mat.type = MaterialType::ROUGH_DIELECTRIC;
	mat.eta = eta;
	mat.alpha = alpha;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

uint32_t OptixScene::addSpecularMicrofacetMaterial(const glm::vec3& f0, const float alpha) {
	_textures.push_back(Texture(f0));
	_textures.push_back(Texture(glm::vec3(alpha)));

	Material mat{};
	mat.type = MaterialType::SPECULAR_MICROFACET;
	mat.color = make_float3(f0.x, f0.y, f0.z);
	mat.colorTexIdx = _textures.size() - 2;
	mat.alpha = alpha;
	mat.alphaTexIdx = _textures.size() - 1;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

uint32_t OptixScene::addSpecularMicrofacetMaterial(const std::string& f0Path, const std::string& alphaPath) {
	_textures.push_back(Texture(f0Path));
	_textures.push_back(Texture(alphaPath));

	Material mat{};
	mat.type = MaterialType::SPECULAR_MICROFACET;
	mat.colorTexIdx = _textures.size() - 2;
	mat.alphaTexIdx = _textures.size() - 1;
	_mats.push_back(mat);
	return _mats.size() - 1;
}

void OptixScene::cornellScene() {
	_cam.setLookat(glm::vec3(0.0f, 0.0f, -2.0f));
	_cam.setPosition(glm::vec3(0.0f, 0.0f, 12.0f));
	_cam.setFOV(60.f);


	auto whiteDiffuse = addLambertDiffuseMaterial(glm::vec3(1.0f));
	auto redDiffuse = addLambertDiffuseMaterial(glm::vec3(1.0f, 0.0f, 0.0f));
	auto greenDiffuse = addLambertDiffuseMaterial(glm::vec3(0.0f, 1.0f, 0.0f));
	auto light = addEmissiveDiffuseMaterial(glm::vec3(15.0f, 13.0f, 10.0f));
	auto mirror = addMirrorMaterial(glm::vec3(1.0f));
	auto glass = addSmoothDielectricMaterial(1.85f);
	auto metal = addSpecularMicrofacetMaterial(glm::vec3(0.560f, 0.570f, 0.580f), 0.1f);
	auto roughD = addRoughDielectricMaterial(1.85f, 0.1f);
	auto texDiffuse = addLambertDiffuseMaterial("assets/textures/testarossa.jpg");

	//cornell box
	{
		// floor
		GeometryData t1 = {};
		t1.triangle = {
			make_float3(-5.0f, -5.0f, 5.0f), // v0
			make_float3(5.0f, -5.0f,  5.0f),  // v1
			make_float3(-5.0f, -5.0f, -5.0f), // v2
			make_float3(0.0f, 1.0f, 0.0f)     //normal
		};
		t1.materialIndex = whiteDiffuse; // materialIndex

		GeometryData t2 = {};
		t2.triangle = {
			make_float3(-5.0f, -5.0f, -5.0f),
			make_float3(5.0f, -5.0f,  5.0f),
			make_float3(5.0f, -5.0f,  -5.0f),
			make_float3(0.0f, 1.0f, 0.0f)
		};
		t2.materialIndex = whiteDiffuse;
		_geometry.push_back(t1);
		_geometry.push_back(t2);

		// ceil
		GeometryData t3 = {};
		t3.triangle = {
			make_float3(-5.0f, 5.0f, 5.0f), // v0
			make_float3(5.0f, 5.0f,  5.0f),  // v1
			make_float3(-5.0f, 5.0f, -5.0f), // v2
			make_float3(0.0f, -1.0f, 0.0f)     //normal
		};
		t3.materialIndex = whiteDiffuse; // materialIndex

		GeometryData t4 = {};
		t4.triangle = {
			make_float3(-5.0f, 5.0f, -5.0f),
			make_float3(5.0f, 5.0f,  5.0f),
			make_float3(5.0f, 5.0f,  -5.0f),
			make_float3(0.0f, -1.0f, 0.0f)
		};
		t4.materialIndex = whiteDiffuse;
		_geometry.push_back(t3);
		_geometry.push_back(t4);

		// left
		GeometryData t5 = {};
		t5.triangle = {
			make_float3(-5.0f, -5.0f, 5.0f), // v0
			make_float3(-5.0f, -5.0f, -5.0f),  // v1
			make_float3(-5.0f, 5.0f, 5.0f), // v2
			make_float3(1.0f, 0.0f, 0.0f)     //normal
		};
		t5.materialIndex = 1; // materialIndex

		GeometryData t6 = {};
		t6.triangle = {
			make_float3(-5.0f, 5.0f, 5.0f),
			make_float3(-5.0f, -5.0f, -5.0f),
			make_float3(-5.0f, 5.0f,  -5.0f),
			make_float3(1.0f, 0.0f, 0.0f)
		};
		t6.materialIndex = 1;
		_geometry.push_back(t5);
		_geometry.push_back(t6);

		// right
		GeometryData t7 = {};
		t7.triangle = {
			make_float3(5.0f, -5.0f, 5.0f), // v0
			make_float3(5.0f, -5.0f, -5.0f),  // v1
			make_float3(5.0f, 5.0f, 5.0f), // v2
			make_float3(-1.0f, 0.0f, 0.0f)     //normal
		};
		t7.materialIndex = 2; // materialIndex

		GeometryData t8 = {};
		t8.triangle = {
			make_float3(5.0f, 5.0f, 5.0f),
			make_float3(5.0f, -5.0f, -5.0f),
			make_float3(5.0f, 5.0f,  -5.0f),
			make_float3(-1.0f, 0.0f, 0.0f)
		};
		t8.materialIndex = 2;
		_geometry.push_back(t7);
		_geometry.push_back(t8);

		// back
		addTriangle(
			glm::vec3(-5.0f, -5.0f, -5.0f),
			glm::vec3(5.0f, -5.0f, -5.0f),
			glm::vec3(-5.0f, 5.0f, -5.0f),
			texDiffuse,
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(0.0f, 1.0f)
		);
		addTriangle(
			glm::vec3(-5.0f, 5.0f, -5.0f),
			glm::vec3(5.0f, -5.0f, -5.0f),
			glm::vec3(5.0f, 5.0f, -5.0f),
			texDiffuse,
			glm::vec2(0.0f, 1.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f)
		);
	}


	//load_obj("assets/models/suzanne.obj", roughD, make_float3(5.0f, -3.0f, -8.4f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/cube.obj", roughD, make_float3(0.0f, -2.99f, 0.0f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/swag.obj", metal, make_float3(0.0f, -1.0f, 0.0f), make_float3(2.0f, 2.0f, 2.0f));
	load_obj("assets/models/sphere.obj", metal, make_float3(0.0f, -2.99f, 1.0f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/teapot.obj", roughD, make_float3(0.0f, -3.0f, -0.0f), make_float3(1.0f, 1.0f, 1.0f));

	// lights
	{
		addTriangleLight(
			glm::vec3(-1.0f, 4.99f, -1.0f), // v0
			glm::vec3(1.0f, 4.99f, -1.0f), // v1
			glm::vec3(-1.0f, 4.99f, 1.0f),  // v2
			light  // materialIndex
		);
		addTriangleLight(
			glm::vec3(-1.0f, 4.99f, 1.0f), // v0
			glm::vec3(1.0f, 4.99f, -1.0f),  // v1
			glm::vec3(1.0f, 4.99f, 1.0f), // v2
			light  // materialIndex
		);

		/*addAreaLight(
			glm::vec3(-1.0f, 4.99f, -1.0f),
			glm::vec3(2.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 2.0f),
			glm::vec3(15.f)
		);*/
	}

}

void OptixScene::sphereScene() {

	// mats
	auto light = addEmissiveDiffuseMaterial(glm::vec3(15.0f, 13.0f, 10.0f));

	auto whiteDiffuse = addLambertDiffuseMaterial(glm::vec3(1.0f));
	auto glass = addSmoothDielectricMaterial(1.85f);
	auto iron = addSpecularMicrofacetMaterial(glm::vec3(0.560f, 0.570f, 0.580f), 0.1f);
	auto gold = addSpecularMicrofacetMaterial(glm::vec3(1.000f, 0.766f, 0.336f), 0.2f);
	auto roughD = addRoughDielectricMaterial(1.85f, 0.1f);
	auto metalTextured = addSpecularMicrofacetMaterial("assets/textures/metal.png", "assets/textures/roughness.png");

	// cam
	_cam.setFOV(60.0f);
	_cam.setPosition(glm::vec3(0.3f, 1.0f, 1.0f));
	_cam.setLookat(glm::vec3(0.03f, -0.2f, 0.2f));

	// light
	addTriangleLight(
		glm::vec3(-0.5f, 0.3f, 1.5f),
		glm::vec3(-0.5f, 0.7f, 1.5f),
		glm::vec3(0.5f, 0.3f, 1.5f),
		light);

	addTriangleLight(
		glm::vec3(0.5f, 0.3f, 1.5f),
		glm::vec3(-0.5f, 0.7f, 1.5f),
		glm::vec3(0.5f, 0.7f, 1.5f),
		light);

	addTriangleLight(
		glm::vec3(2.0f, 0.7f, 0.5f),
		glm::vec3(2.0f, 0.3f, 0.5f),
		glm::vec3(2.0f, 0.3f, -0.5f),
		light);

	addTriangleLight(
		glm::vec3(-0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, 0.2f),
		glm::vec3(-0.2f, 1.49f, 0.2f),
		light);

	addTriangleLight(
		glm::vec3(-0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, 0.2f),
		light);

	// floor
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(-100.5f, -0.5f, 50.5f),
		glm::vec3(100.5f, -0.5f, 50.5f),
		whiteDiffuse
	);
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(100.5f, -0.5f, 50.5f),
		glm::vec3(100.5f, -0.5f, -100.5f),
		whiteDiffuse
	);

	load_obj("assets/models/sphere.obj", metalTextured, make_float3(-0.36f, -0.2f, -0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", iron, make_float3(0.36f, -0.2f, -0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", roughD, make_float3(-0.36f, -0.2f, 0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", glass, make_float3(0.36f, -0.2f, 0.36f), make_float3(0.3f, 0.3f, 0.3f));

}

void OptixScene::dragonScene() {

	_cam.setLookat(glm::vec3(0.0f, 0.0f, -2.0f));
	_cam.setPosition(glm::vec3(20.0f, 20.0f, -90.0f));
	_cam.setFOV(60.f);

	auto whiteDiffuse = addLambertDiffuseMaterial(glm::vec3(1.0f));
	auto glass = addSmoothDielectricMaterial(1.35f);
	auto iron = addSpecularMicrofacetMaterial(glm::vec3(0.560f, 0.570f, 0.580f), 0.1f);
	auto gold = addSpecularMicrofacetMaterial(glm::vec3(1.000f, 0.766f, 0.336f), 0.2f);
	auto roughD = addRoughDielectricMaterial(1.85f, 0.05f);

	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(-100.5f, -0.5f, 100.5f),
		glm::vec3(100.5f, -0.5f, 100.5f),
		whiteDiffuse
	);
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(100.5f, -0.5f, 100.5f),
		glm::vec3(100.5f, -0.5f, -100.5f),
		whiteDiffuse
	);

	load_obj("assets/models/xyzrgb_dragon.obj", gold, make_float3(0.0f, 20.0f, 0.0f), make_float3(0.5f, 0.5f, 0.5f));

}

void OptixScene::glassScene() {

	_cam.setLookat(glm::vec3(0.0f, -1.1f, 0.6f));
	_cam.setPosition(glm::vec3(6.0f, 0.0f, 0.0f));
	_cam.setFOV(45.f);

	auto light = addEmissiveDiffuseMaterial(glm::vec3(50.f));
	auto whiteDiffuse = addLambertDiffuseMaterial(glm::vec3(1.0f));
	auto glass = addSmoothDielectricMaterial(1.55f);
	auto allu = addSpecularMicrofacetMaterial(glm::vec3(0.913f, 0.921f, 0.925f), 0.15f);
	auto roughD = addRoughDielectricMaterial(1.55f, 0.35f);


	addTriangleLight(
		glm::vec3(5.5f, 0.3f, 1.5f),
		glm::vec3(5.5f, 0.9f, 1.5f),
		glm::vec3(5.5f, 0.3f, -1.5f),
		light);
	addTriangleLight(
		glm::vec3(5.5f, 0.9f, 1.5f),
		glm::vec3(5.5f, 0.9f, -1.5f),
		glm::vec3(5.5f, 0.3f, -1.5f),
		light);

	addTriangleLight(
		glm::vec3(0.5f, 0.3f, 4.5f),
		glm::vec3(0.5f, 0.9f, 4.5f),
		glm::vec3(1.5f, 0.3f, 4.5f),
		light);
	addTriangleLight(
		glm::vec3(1.5f, 0.3f, 4.5f),
		glm::vec3(0.5f, 0.9f, 4.5f),
		glm::vec3(1.5f, 0.9f, 4.5f),
		light);

	load_obj("assets/models/glass.obj", glass, make_float3(0.0f, -3.06f, 1.0f), make_float3(1.5f, 1.5f, 1.5f));
	load_obj("assets/models/bend_plane.obj", whiteDiffuse, make_float3(0.0f, -2.99f, 1.0f), make_float3(1.5f, 1.5f, 1.5f));
}

void OptixScene::loadEnvMap(std::string name) {
	for (auto m : light::envMaps) {
		if (m.name == name)
			_environment = m;
	}
	// TODO: maybe add special treatment if not found
}