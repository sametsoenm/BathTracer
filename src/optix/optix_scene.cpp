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
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const uint32_t matIdx) {

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
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const uint32_t matIdx) {
	_geometry.push_back(createTriangle(v0, v1, v2, matIdx));
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

void OptixScene::cornellScene() {
	_cam.setLookat(glm::vec3(0.0f, 0.0f, -2.0f));
	_cam.setPosition(glm::vec3(0.0f, 0.0f, 12.0f));
	_cam.setFOV(60.f);

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
		t1.materialIndex = 0; // materialIndex

		GeometryData t2 = {};
		t2.triangle = {
			make_float3(-5.0f, -5.0f, -5.0f),
			make_float3(5.0f, -5.0f,  5.0f),
			make_float3(5.0f, -5.0f,  -5.0f),
			make_float3(0.0f, 1.0f, 0.0f)
		};
		t2.materialIndex = 0;
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
		t3.materialIndex = 0; // materialIndex

		GeometryData t4 = {};
		t4.triangle = {
			make_float3(-5.0f, 5.0f, -5.0f),
			make_float3(5.0f, 5.0f,  5.0f),
			make_float3(5.0f, 5.0f,  -5.0f),
			make_float3(0.0f, -1.0f, 0.0f)
		};
		t4.materialIndex = 0;
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
		GeometryData t9 = {};
		t9.triangle = {
			make_float3(-5.0f, -5.0f, -5.0f), // v0
			make_float3(5.0f, -5.0f, -5.0f),  // v1
			make_float3(-5.0f, 5.0f, -5.0f), // v2
			make_float3(0.0f, 0.0f, 1.0f)     //normal
		};
		t9.materialIndex = 0; // materialIndex

		GeometryData t10 = {};
		t10.triangle = {
			make_float3(-5.0f, 5.0f, -5.0f),
			make_float3(5.0f, -5.0f, -5.0f),
			make_float3(5.0f, 5.0f,  -5.0f),
			make_float3(0.0f, 0.0f, 1.0f)
		};
		t10.materialIndex = 0;
		_geometry.push_back(t9);
		_geometry.push_back(t10);
	}


	//load_obj("assets/models/suzanne.obj", 7, make_float3(5.0f, -3.0f, -8.4f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/cube.obj", 7, make_float3(0.0f, -2.99f, 0.0f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/swag.obj", 6, make_float3(0.0f, -1.0f, 0.0f), make_float3(2.0f, 2.0f, 2.0f));
	load_obj("assets/models/sphere.obj", 6, make_float3(0.0f, -2.99f, 1.0f), make_float3(2.0f, 2.0f, 2.0f));
	//load_obj("assets/models/teapot.obj", 7, make_float3(0.0f, -3.0f, -0.0f), make_float3(1.0f, 1.0f, 1.0f));

	// lights
	{
		addTriangleLight(
			glm::vec3(-1.0f, 4.99f, -1.0f), // v0
			glm::vec3(1.0f, 4.99f, -1.0f), // v1
			glm::vec3(-1.0f, 4.99f, 1.0f),  // v2
			3  // materialIndex
		);
		addTriangleLight(
			glm::vec3(-1.0f, 4.99f, 1.0f), // v0
			glm::vec3(1.0f, 4.99f, -1.0f),  // v1
			glm::vec3(1.0f, 4.99f, 1.0f), // v2
			3  // materialIndex
		);

		/*addAreaLight(
			glm::vec3(-1.0f, 4.99f, -1.0f),
			glm::vec3(2.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 2.0f),
			glm::vec3(15.f)
		);*/
	}

	Material whiteDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(1.0f, 1.0f, 1.0f) // albedo
	};

	Material redDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(1.0f, 0.0f, 0.0f)
	};

	Material greenDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(0.0f, 1.0f, 0.0f)
	};

	Material light = {};
	light.type = MaterialType::EMISSIVE_DIFFUSE;
	light.emission = make_float3(15.0f, 13.0f, 10.0f);

	Material mirror{};
	mirror.type = MaterialType::MIRROR;
	mirror.isDelta = true;
	mirror.reflectance = make_float3(1.0f, 1.0f, 1.0f);

	Material glass{};
	glass.type = MaterialType::SMOOTH_DIELECTRIC;
	glass.isDelta = true;
	glass.eta = 1.85f;

	Material metal{};
	metal.type = MaterialType::SPECULAR_MICROFACET;
	metal.color = make_float3(0.560f, 0.570f, 0.580f);
	metal.alpha = 0.1f;

	Material roughD{};
	roughD.type = MaterialType::ROUGH_DIELECTRIC;
	roughD.eta = 1.85f;
	roughD.alpha = 0.1f;

	_mats.push_back(whiteDiffuse);
	_mats.push_back(redDiffuse);
	_mats.push_back(greenDiffuse);
	_mats.push_back(light);
	_mats.push_back(mirror);
	_mats.push_back(glass);
	_mats.push_back(metal);
	_mats.push_back(roughD);

}

void OptixScene::sphereScene() {

	// mats
	Material light = {};
	light.type = MaterialType::EMISSIVE_DIFFUSE;
	light.emission = make_float3(15.0f, 13.0f, 10.0f);
	_mats.push_back(light);

	Material whiteDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(1.0f, 1.0f, 1.0f) // albedo
	};
	_mats.push_back(whiteDiffuse);

	Material glass{};
	glass.type = MaterialType::SMOOTH_DIELECTRIC;
	glass.isDelta = true;
	glass.eta = 1.85f;
	_mats.push_back(glass);

	Material iron{};
	iron.type = MaterialType::SPECULAR_MICROFACET;
	iron.color = make_float3(0.560f, 0.570f, 0.580f);
	iron.alpha = 0.1f;
	_mats.push_back(iron);

	Material gold{};
	gold.type = MaterialType::SPECULAR_MICROFACET;
	gold.color = make_float3(1.000f, 0.766f, 0.336f);
	gold.alpha = 0.2f;
	_mats.push_back(gold);

	Material roughD{};
	roughD.type = MaterialType::ROUGH_DIELECTRIC;
	roughD.eta = 1.85f;
	roughD.alpha = 0.1f;
	_mats.push_back(roughD);

	// cam
	_cam.setFOV(60.0f);
	_cam.setPosition(glm::vec3(0.3f, 1.0f, 1.0f));
	_cam.setLookat(glm::vec3(0.03f, -0.2f, 0.2f));

	// light
	addTriangleLight(
		glm::vec3(-0.5f, 0.3f, 1.5f),
		glm::vec3(-0.5f, 0.7f, 1.5f),
		glm::vec3(0.5f, 0.3f, 1.5f),
		0);

	addTriangleLight(
		glm::vec3(0.5f, 0.3f, 1.5f),
		glm::vec3(-0.5f, 0.7f, 1.5f),
		glm::vec3(0.5f, 0.7f, 1.5f),
		0);

	addTriangleLight(
		glm::vec3(2.0f, 0.7f, 0.5f),
		glm::vec3(2.0f, 0.3f, 0.5f),
		glm::vec3(2.0f, 0.3f, -0.5f),
		0);

	addTriangleLight(
		glm::vec3(-0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, 0.2f),
		glm::vec3(-0.2f, 1.49f, 0.2f),
		0);

	addTriangleLight(
		glm::vec3(-0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, -0.2f),
		glm::vec3(0.2f, 1.49f, 0.2f),
		0);

	// floor
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(-100.5f, -0.5f, 50.5f),
		glm::vec3(100.5f, -0.5f, 50.5f),
		1
	);
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(100.5f, -0.5f, 50.5f),
		glm::vec3(100.5f, -0.5f, -100.5f),
		1
	);

	load_obj("assets/models/sphere.obj", 4, make_float3(-0.36f, -0.2f, -0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", 3, make_float3(0.36f, -0.2f, -0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", 5, make_float3(-0.36f, -0.2f, 0.36f), make_float3(0.3f, 0.3f, 0.3f));
	load_obj("assets/models/sphere.obj", 2, make_float3(0.36f, -0.2f, 0.36f), make_float3(0.3f, 0.3f, 0.3f));

}

void OptixScene::dragonScene() {

	_cam.setLookat(glm::vec3(0.0f, 0.0f, -2.0f));
	_cam.setPosition(glm::vec3(20.0f, 20.0f, -90.0f));
	_cam.setFOV(60.f);

	Material whiteDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(1.0f, 1.0f, 1.0f) // albedo
	};
	_mats.push_back(whiteDiffuse);

	Material glass{};
	glass.type = MaterialType::SMOOTH_DIELECTRIC;
	glass.isDelta = true;
	glass.eta = 1.35f;
	_mats.push_back(glass);

	Material iron{};
	iron.type = MaterialType::SPECULAR_MICROFACET;
	iron.color = make_float3(0.560f, 0.570f, 0.580f);
	iron.alpha = 0.1f;
	_mats.push_back(iron);

	Material gold{};
	gold.type = MaterialType::SPECULAR_MICROFACET;
	gold.color = make_float3(1.000f, 0.766f, 0.336f);
	gold.alpha = 0.2f;
	_mats.push_back(gold);

	Material roughD{};
	roughD.type = MaterialType::ROUGH_DIELECTRIC;
	roughD.eta = 1.85f;
	roughD.alpha = 0.05f;
	_mats.push_back(roughD);

	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(-100.5f, -0.5f, 100.5f),
		glm::vec3(100.5f, -0.5f, 100.5f),
		0
	);
	addTriangle(
		glm::vec3(-100.5f, -0.5f, -100.5f),
		glm::vec3(100.5f, -0.5f, 100.5f),
		glm::vec3(100.5f, -0.5f, -100.5f),
		0
	);

	load_obj("assets/models/xyzrgb_dragon.obj", 3, make_float3(0.0f, 20.0f, 0.0f), make_float3(0.5f, 0.5f, 0.5f));

}

void OptixScene::glassScene() {

	_cam.setLookat(glm::vec3(0.0f, -1.1f, 0.6f));
	_cam.setPosition(glm::vec3(6.0f, 0.0f, 0.0f));
	_cam.setFOV(45.f);

	addTriangleLight(
		glm::vec3(5.5f, 0.3f, 1.5f),
		glm::vec3(5.5f, 0.9f, 1.5f),
		glm::vec3(5.5f, 0.3f, -1.5f),
		0);
	addTriangleLight(
		glm::vec3(5.5f, 0.9f, 1.5f),
		glm::vec3(5.5f, 0.9f, -1.5f),
		glm::vec3(5.5f, 0.3f, -1.5f),
		0);

	addTriangleLight(
		glm::vec3(0.5f, 0.3f, 4.5f),
		glm::vec3(0.5f, 0.9f, 4.5f),
		glm::vec3(1.5f, 0.3f, 4.5f),
		0);
	addTriangleLight(
		glm::vec3(1.5f, 0.3f, 4.5f),
		glm::vec3(0.5f, 0.9f, 4.5f),
		glm::vec3(1.5f, 0.9f, 4.5f),
		0);

	Material light = {};
	light.type = MaterialType::EMISSIVE_DIFFUSE;
	light.emission = make_float3(50.0f, 50.0f, 50.0f);
	_mats.push_back(light);

	Material whiteDiffuse = {
		MaterialType::LAMBERT_DIFFUSE,
		make_float3(1.0f, 1.0f, 1.0f) // albedo
	};
	_mats.push_back(whiteDiffuse);

	Material glass{};
	glass.type = MaterialType::SMOOTH_DIELECTRIC;
	glass.isDelta = true;
	glass.eta = 1.55f;
	_mats.push_back(glass);

	Material roughD{};
	roughD.type = MaterialType::ROUGH_DIELECTRIC;
	roughD.eta = 1.55f;
	roughD.alpha = 0.35f;
	_mats.push_back(roughD);

	Material allu{};
	allu.type = MaterialType::SPECULAR_MICROFACET;
	allu.color = make_float3(0.913f, 0.921f, 0.925f);
	allu.alpha = 0.15f;
	_mats.push_back(allu);

	load_obj("assets/models/glass.obj", 2, make_float3(0.0f, -3.06f, 1.0f), make_float3(1.5f, 1.5f, 1.5f));
	load_obj("assets/models/bend_plane.obj", 1, make_float3(0.0f, -2.99f, 1.0f), make_float3(1.5f, 1.5f, 1.5f));
}

void OptixScene::loadEnvMap(std::string name) {
	for (auto m : light::envMaps) {
		if (m.name == name)
			_environment = m;
	}
	// TODO: maybe add special treatment if not found
}