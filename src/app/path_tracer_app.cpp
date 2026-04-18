#include "path_tracer_app.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "gui/imgui.h"

#include <chrono>
#include <iostream>

PathTracerApp::PathTracerApp() :
	_window(settings::IMAGE_WIDTH, settings::IMAGE_HEIGHT, "BathTracer"),
	_scenes(),
	_display(settings::IMAGE_WIDTH, settings::IMAGE_HEIGHT),
	_gui(_window.handle()),
	_optixRenderer(nullptr) { 

	_scenes.push_back({
		std::string(types::to_string(types::SceneType::CORNELL_SCENE)),
		std::make_unique<OptixScene>(types::SceneType::CORNELL_SCENE)
		});
	_scenes.push_back({
		std::string(types::to_string(types::SceneType::SPHERES_SCENE)),
		std::make_unique<OptixScene>(types::SceneType::SPHERES_SCENE)
		});
	_scenes.push_back({
		std::string(types::to_string(types::SceneType::DRAGON_SCENE)),
		std::make_unique<OptixScene>(types::SceneType::DRAGON_SCENE)
		});
	_scenes.push_back({
		std::string(types::to_string(types::SceneType::GLASS_SCENE)),
		std::make_unique<OptixScene>(types::SceneType::GLASS_SCENE)
		});
	_scenes.push_back({
		std::string(types::to_string(types::SceneType::EMPTY_SCENE)),
		std::make_unique<OptixScene>(types::SceneType::EMPTY_SCENE)
		});
	_activeSceneIdx = 0;

	_optixRenderer.setScene(_scenes[_activeSceneIdx].scene.get());

	_activeRenderer = &_optixRenderer;
	_activeRendererType = _activeRenderer->getType();
}

void PathTracerApp::init() {
	_activeRenderer->init(settings::IMAGE_WIDTH, settings::IMAGE_HEIGHT);
}

void PathTracerApp::run() {

	init();

	while (!_window.shouldClose()) {

		processInput();

		auto t0 = Clock::now();
		update();
		auto t1 = Clock::now();
		_stats.renderTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

		t0 = Clock::now();
		render();
		t1 = Clock::now();
		_stats.displayTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

		_window.swapBuffers();
		_window.pollEvents();
	}

	_activeRenderer->shutdown();
}

void PathTracerApp::processInput() {
	if (glfwGetKey(_window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		_window.setShouldClose(true);
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return;
	}

	bool cameraChanged = false;

	const float scrollY = _window.consumeScrollOffsetY();
	if (scrollY != 0.0f) {
		_scenes[_activeSceneIdx].scene->cam().zoom(scrollY);
		cameraChanged = true;
	}

	double mouseDx = 0.0;
	double mouseDy = 0.0;
	if (_window.consumeMouseDelta(mouseDx, mouseDy)) {
		constexpr float rotationSpeed = 0.005f;

		const float yaw = -static_cast<float>(mouseDx) * rotationSpeed;
		const float pitch = -static_cast<float>(mouseDy) * rotationSpeed;

		if (_window.isLeftMouseButtonDown()) {
			_scenes[_activeSceneIdx].scene->cam().orbit(yaw, pitch);
			cameraChanged = true;
		}
		else if (_window.isRightMouseButtonDown()) {
			_scenes[_activeSceneIdx].scene->cam().look(yaw, pitch);
			cameraChanged = true;
		}
		_activeRenderer->setIsRendering(true);
	}

	if (cameraChanged) {
		_activeRenderer->resetAccumulation();
	}
}

void PathTracerApp::update() {

	if (_activeRenderer->getType() != _activeRendererType) {
		setActiveRenderer(_activeRendererType);
		_activeRenderer->resetAccumulation();
		_activeRenderer->setIsRendering(true);
	}
	if (_sceneHasChanged) {
		setActiveScene(_activeSceneIdx);
		_sceneHasChanged = false;
		_activeRenderer->resetAccumulation();
		_activeRenderer->reloadScene();
		_activeRenderer->setIsRendering(true);
	}
	_activeRenderer->render();
}

void PathTracerApp::render() {
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	_gui.beginFrame();

	types::RendererType nextType = _activeRendererType;
	size_t nextSceneIdx = _activeSceneIdx;
	_gui.draw(_stats, *_activeRenderer, nextType, _scenes, nextSceneIdx);
	if (nextType != _activeRendererType)
		_activeRendererType = nextType;
	if (nextSceneIdx != _activeSceneIdx) {
		_activeSceneIdx = nextSceneIdx;
		_sceneHasChanged = true;
	}


	_display.display(_activeRenderer->buffer().data());

	_gui.endFrame();

}

void PathTracerApp::setActiveRenderer(const types::RendererType type) {
	switch (type) {
	case types::RendererType::OPTIX:
		_activeRenderer = &_optixRenderer;
		break;
	default:
		_activeRenderer = &_optixRenderer;
	}
}

void PathTracerApp::setActiveScene(const size_t idx) {
	_optixRenderer.setScene(_scenes[idx].scene.get());

}