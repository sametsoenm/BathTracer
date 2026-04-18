#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/trigonometric.hpp>

#include "device_types.h"
#include "rendersettings.h"

class Camera {
public:
	Camera() :
		_origin(0.0f, 0.0f, 1.0f),
		_lookat(glm::vec3(0.0f)),
		_up(glm::vec3(0.0f, 1.0f, 0.0f)),
		_focusDist(1.0f),
		_lensRadius(0.0f),
		_vfov(80.0f) {
		updateCamera();
	}

	Camera(
		const glm::vec3& __origin, 
		const glm::vec3& lookat,
		const glm::vec3& up,
		float __focusDist,
		float __lensRadius,
		float __vfov) :
		_origin(__origin),
		_lookat(lookat),
		_up(up),
		_focusDist(__focusDist),
		_lensRadius(__lensRadius),
		_vfov(__vfov) {
		updateCamera();
	}

	CameraData getDeviceData() const;
	void updateCamera();

	float fov() const { return _vfov; }
	float blurRadius() const { return _lensRadius; }
	float focusDist() const { return _focusDist; }

	void setPosition(const glm::vec3& __pos) { 
		_origin = __pos;
		updateCamera();
	}
	void setLookat(const glm::vec3& __pos) {
		_lookat = __pos;
		updateCamera();
	}
	void setFOV(const float __fov) { 
		_vfov = __fov;
		updateCamera();
	}
	void setFocusDist(const float __focusDist) {
		_focusDist = __focusDist;
		updateCamera();
	}
	void setBlurRadius(const float __lensRadius) {
		_lensRadius = __lensRadius;
		updateCamera();
	}

	void zoom(const float scroll);
	void orbit(const float yaw, const float pitch); // rotate around lookat
	void look(float deltaYaw, float deltaPitch); // rotate around origin

	const glm::vec3& origin() const { return _origin; }
	const glm::vec3& lookAt() const { return _lookat; }

private:
	glm::vec3 _origin;
	glm::vec3 _lookat;
	glm::vec3 _up;
	
	glm::vec3 _u;
	glm::vec3 _v;
	glm::vec3 _w;

	float _viewportWidth;
	float _viewportHeight;
	float _focusDist;
	float _lensRadius;
	float _vfov;

};