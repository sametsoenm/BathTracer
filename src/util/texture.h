#pragma once

#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include "util/constants.h"

namespace texture {

	enum class WrapMode {
		CLAMP,
		REPEAT,
		MIRROR
	};

	enum class FilterMode {
		NEAREST,
		LINEAR
	};

	enum class ColorSpace {
		LINEAR,
		SRGB
	};

	inline void wrap(float& x, WrapMode mode) {
		switch (mode) {
		case WrapMode::CLAMP:
			x = glm::clamp(x, 0.0f, 1.0f);
			break;
		case WrapMode::REPEAT:
			x = x - std::floor(x);
			break;
		case WrapMode::MIRROR:
			x = 1.0f - std::abs(std::fmod(x, 2.0f) - 1.0f);
			break;
		}
	}

	inline void wrap(glm::vec2& uv, WrapMode mode) {
		switch (mode) {
		case WrapMode::CLAMP:
			uv.x = glm::clamp(uv.x, 0.0f, 1.0f);
			uv.y = glm::clamp(uv.y, 0.0f, 1.0f);
			break;
		case WrapMode::REPEAT:
			uv.x = uv.x - std::floor(uv.x);
			uv.y = uv.y - std::floor(uv.y);
			break;
		case WrapMode::MIRROR:
			uv.x = 1.0f - std::abs(std::fmod(uv.x, 2.0f) - 1.0f);
			uv.y = 1.0f - std::abs(std::fmod(uv.y, 2.0f) - 1.0f);
			break;
		}
	}

	inline glm::vec2 directionToUV(const glm::vec3& dir) {
		glm::vec3 d = glm::normalize(dir);

		const float phi = std::atan2(d.z, d.x);
		const float theta = std::acos(glm::clamp(d.y, -1.0f, 1.0f));

		const float u = (phi + math::PI) / (2.0f * math::PI);
		const float v = theta / math::PI;

		return glm::vec2(u, v);
	}

}

class Texture {
public:

	Texture() = default;

	Texture(const std::string& filename,
		bool flip_vertical = false,
		const texture::ColorSpace cs = texture::ColorSpace::SRGB) {
		load(filename, flip_vertical, cs);
	}

	// one color texture
	Texture(const glm::vec3& color) : _width(1), _height(1), 
		_filterMode(texture::FilterMode::NEAREST) {
		_pixels.push_back(color);
	}

	bool load(const std::string& filename,
		bool flip_vertical = false,
		const texture::ColorSpace cs = texture::ColorSpace::SRGB);

	glm::vec3 sample(float u, float v) const;
	glm::vec3 sampleNearest(float u, float v) const;

	void setWrapModeU(texture::WrapMode __mode) { _wrapU = __mode; }
	void setWrapModeV(texture::WrapMode __mode) { _wrapV = __mode; }

	bool valid() const { return !_pixels.empty(); }

	std::vector<glm::vec3> pixels() const { return _pixels; }
	int width() const { return _width; }
	int height() const { return _height; }

private:

	int _width = 0;
	int _height = 0;
	int _channels = 0;
	bool _hdr = false;

	texture::WrapMode _wrapU = texture::WrapMode::REPEAT;
	texture::WrapMode _wrapV = texture::WrapMode::REPEAT;

	// TODO: use
	texture::FilterMode _filterMode = texture::FilterMode::LINEAR;

	std::vector<glm::vec3> _pixels;
};



