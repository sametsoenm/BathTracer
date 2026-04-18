#pragma once

#include <cstddef>
#include <limits>

#include "core/pt_types.h"

namespace settings {

	constexpr const char* FILE_NAME = "output.png";

	constexpr size_t IMAGE_WIDTH = 768;
	constexpr size_t IMAGE_HEIGHT = 768;
	constexpr float ASPECT_RATIO = 
		static_cast<float>(IMAGE_WIDTH) / static_cast<float>(IMAGE_HEIGHT);

	constexpr float T_MIN = 1e-3f;
	constexpr float T_MAX = std::numeric_limits<float>::max();

	constexpr size_t MAX_RAY_DEPTH = 10;
	constexpr size_t SAMPLES_PER_FRAME = 16;

	constexpr bool FOGGY = false;
	constexpr float FOG_DENSITY = 0.1f;
	constexpr bool ENV_MAP = true;
	constexpr const char* ENV_MAP_ON_START = "Indoor";

	constexpr types::ShadingMode SHADING_MODE = types::ShadingMode::FLAT;

}