#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <algorithm>
#include <vector_functions.hpp>

void Camera::updateCamera() {

	const float theta = glm::radians(_vfov);
	const float h = glm::tan(0.5f * theta);
	_viewportHeight = 2.0f * h * _focusDist;
	_viewportWidth = _viewportHeight * settings::ASPECT_RATIO;

	_w = glm::normalize(_origin - _lookat);
	_u = glm::normalize(glm::cross(_up, _w));
	_v = glm::cross(_w, _u);
}

CameraData Camera::getDeviceData() const {
	CameraData data{};
	data.origin = make_float3(_origin.x, _origin.y, _origin.z);
	data.u = make_float3(_u.x, _u.y, _u.z);
	data.v = make_float3(_v.x, _v.y, _v.z);
	data.w = make_float3(_w.x, _w.y, _w.z);
	data.viewportWidth = _viewportWidth;
	data.viewportHeight = _viewportHeight;
	data.focusDist = _focusDist;
	data.lensRadius = _lensRadius;
	return data;
}

void Camera::zoom(const float scroll) {

	_vfov -= scroll * 2.0f;
	_vfov = glm::max(0.0f, _vfov);

	updateCamera();
}

void Camera::orbit(const float yaw, const float pitch) {

	glm::vec3 offset = _origin - _lookat;
	const float radius = glm::length(offset);
	if (radius <= 0.0001f) {
		return;
	}

	offset = glm::rotate(offset, yaw, _up);
	glm::vec3 forward = glm::normalize(-offset);
	glm::vec3 right = glm::normalize(glm::cross(forward, _up));
	if (glm::length(right) <= 0.0001f) {
		return;
	}

	glm::vec3 pitchedOffset = glm::rotate(offset, pitch, right);
	glm::vec3 newForward = glm::normalize(_lookat - (_lookat + pitchedOffset));
	const float alignment = glm::abs(glm::dot(newForward, _up));
	if (alignment < 0.999f) {
		offset = pitchedOffset;
	}

	_origin = _lookat + glm::normalize(offset) * radius;

	updateCamera();
}

void Camera::look(float deltaYaw, float deltaPitch) {

	glm::vec3 forward = _lookat - _origin;
	const float distance = glm::length(forward);
	if (distance <= 0.0001f) {
		return;
	}

	forward = glm::rotate(forward, deltaYaw, _up);
	glm::vec3 dir = glm::normalize(forward);
	glm::vec3 right = glm::normalize(glm::cross(dir, _up));
	if (glm::length(right) <= 0.0001f) {
		return;
	}

	glm::vec3 pitchedForward = glm::rotate(forward, deltaPitch, right);
	glm::vec3 newDir = glm::normalize(pitchedForward);
	const float alignment = glm::abs(glm::dot(newDir, _up));
	if (alignment < 0.999f) {
		forward = pitchedForward;
	}

	_lookat = _origin + glm::normalize(forward) * distance;

	updateCamera();
}