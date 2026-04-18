#pragma once

#include <vector_types.h>
#include <string_view>

#include <util/texture.h>

namespace types {

	enum class RenderStrategy {
		SINGLE_THREAD,
		MULTI_THREAD_FIXED_ROWS,
		MULTI_THREAD_ATOMIC_ROWS,
		MULTI_THREAD_TILES,
		INTERACTIVE
	};

	enum class RendererType {
		OPTIX
		// after adding ne type also adjust the corresp. array!!!
	};

	enum class SceneType {
		CORNELL_SCENE,
		SPHERES_SCENE,
		DRAGON_SCENE,
		GLASS_SCENE,
		EMPTY_SCENE
		// after adding ne type also adjust the corresp. array!!!
	};

	enum class ShadingMode {
		FLAT,
		PHONG,
		GOURAUD
	};

	constexpr std::string_view to_string(RendererType t) {
		switch (t) {
		case RendererType::OPTIX: return "Optix";
		default: return "Unkown";
		}
	}

	constexpr std::string_view to_string(SceneType t) {
		switch (t) {
		case SceneType::CORNELL_SCENE: return "Cornell box";
		case SceneType::SPHERES_SCENE: return "Spheres";
		case SceneType::DRAGON_SCENE: return "Dragon";
		case SceneType::GLASS_SCENE: return "Glass";
		case SceneType::EMPTY_SCENE: return "Empty";
		default: return "Unkown";
		}
	}

	struct RenderStatistics {
		double renderTimeMs = 0.0;
		double displayTimeMs = 0.0;
		double frameTimeMs = 0.0;
		double fps = 0.0;
	};

}

namespace light {

	struct EnvironmentMap {
		std::string name;
		Texture map;
	};

	inline std::vector<EnvironmentMap> envMaps = {
		{"Indoor", Texture("assets/textures/envmap01.hdr", false, texture::ColorSpace::LINEAR)},
		{"Nature", Texture("assets/textures/envmap02.hdr", false, texture::ColorSpace::LINEAR)},
		{"Studio", Texture("assets/textures/envmap03.hdr", false, texture::ColorSpace::LINEAR)},
	};

	inline int getEnvmapIdx(std::string name) {
		for (int i = 0; i < envMaps.size(); i++) {
			if (name == envMaps[i].name)
				return i;
		}
		return 0;
	}

}