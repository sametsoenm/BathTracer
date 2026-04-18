#pragma once

#include "platform/window.h"
#include "platform/display.h"
#include "optix/optix_renderer.h"
#include "gui/gui_layer.h"
#include "rendersettings.h"

#include <chrono>

class PathTracerApp {

	using Clock = std::chrono::steady_clock;

public:
	PathTracerApp();
	void run();

private:
	void init();
	void processInput();
	void update();
	void render();

	void setActiveRenderer(const types::RendererType type);
	void setActiveScene(const size_t idx);

private:
	Window _window;
	Display _display;
	GuiLayer _gui;

	size_t _activeSceneIdx = 0;
	bool _sceneHasChanged = false;
	std::vector<SceneEntry> _scenes;
	OptixRenderer* _activeRenderer = nullptr;
	types::RendererType _activeRendererType;
	OptixRenderer _optixRenderer;

	types::RenderStatistics _stats;
};