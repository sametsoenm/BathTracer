#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "util/image.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <iostream>

using namespace texture;

bool Texture::load(const std::string& filename, bool flip_vertical, const ColorSpace cs) {
	stbi_set_flip_vertically_on_load(flip_vertical ? 1 : 0);

	_width = _height = _channels = 0;
	_hdr = stbi_is_hdr(filename.c_str()) != 0;
	_pixels.clear();

	if (_hdr) {
		float* data = stbi_loadf(filename.c_str(), &_width, &_height, &_channels, 0);
		if (!data) {
			std::cout << "couldnt load image hdr: " << filename << "\n";
			std::cout << "reason: " << stbi_failure_reason() << "\n";
			return false;
		}

		_pixels.resize(_width * _height);

		for (int y = 0; y < _height; ++y) {
			for (int x = 0; x < _width; ++x) {
				const int idx = (y * _width + x) * _channels;

				const float r = data[idx + 0];
				const float g = (_channels > 1) ? data[idx + 1] : r;
				const float b = (_channels > 2) ? data[idx + 2] : r;

				_pixels[y * _width + x] = glm::vec3(r, g, b);
			}
		}

		stbi_image_free(data);
		return true;
	}

	unsigned char* data = stbi_load(filename.c_str(), &_width, &_height, &_channels, 0);
	if (!data) {
		std::cout << "couldnt load image ldr: " << filename << "\n";
		std::cout << "reason: " << stbi_failure_reason() << "\n";
		return false;
	}

	_pixels.resize(_width * _height);

	for (int y = 0; y < _height; ++y) {
		for (int x = 0; x < _width; ++x) {
			const int idx = (y * _width + x) * _channels;

			float r = data[idx + 0] / 255.0f;
			float g = (_channels > 1) ? data[idx + 1] / 255.0f : r;
			float b = (_channels > 2) ? data[idx + 2] / 255.0f : r;

			// convert SRGB input to linear
			if (cs == ColorSpace::SRGB) {
				r = image::srgb_to_linear(r);
				g = image::srgb_to_linear(g);
				b = image::srgb_to_linear(b);
			}

			_pixels[y * _width + x] = glm::vec3(r, g, b);
		}
	}

	stbi_image_free(data);
	return true;
}

glm::vec3 Texture::sampleNearest(float u, float v) const {
	if (_pixels.empty())
		return glm::vec3(0.0f);

	wrap(u, _wrapU);
	wrap(v, _wrapV);

	const size_t x = static_cast<size_t>(u * (_width - 1));
	const size_t y = static_cast<size_t>(v * (_width - 1));

	return _pixels[y * _width + x];
}

glm::vec3 Texture::sample(float u, float v) const {
	if (_pixels.empty())
		return glm::vec3(0.0f);

	wrap(u, _wrapU);
	wrap(v, _wrapV);

	const float x = u * (_width - 1);
	const float y = v * (_height - 1);

	const size_t x0 = static_cast<size_t>(std::floor(x));
	const size_t y0 = static_cast<size_t>(std::floor(y));
	const size_t x1 = (x0 + 1) % _width;
	const size_t y1 = (y0 + 1) % _height;

	const float tx = x - static_cast<float>(x0);
	const float ty = y - static_cast<float>(y0);

	const glm::vec3 c0 = 
		glm::mix(_pixels[_width * y0 + x0], _pixels[_width * y0 + x1], tx);
	const glm::vec3 c1 = 
		glm::mix(_pixels[_width * y1 + x0], _pixels[_width * y1 + x1], tx);
	return glm::mix(c0, c1, ty);
}