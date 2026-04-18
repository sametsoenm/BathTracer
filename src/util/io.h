#pragma once

#include <string>
#include <vector>
#include <vector_types.h>

#include <glm/vec3.hpp>

namespace io {
	void output_image(const std::string& filename, 
		const uchar4* fb, 
		const size_t width, const size_t height);

	std::string readTextFile(const std::string& path);

}