#pragma once

#include "rendersettings.h"
#include "optix/optix_scene.h"

#include <vector>

struct GLFWwindow;

class OptixRenderer;
//struct SceneEntry;

class GuiLayer {
public:
    GuiLayer(GLFWwindow* window);
    ~GuiLayer();

    void beginFrame();
    void draw(const types::RenderStatistics& stats, 
        OptixRenderer& renderer, types::RendererType& activeRenderType,
        const std::vector<SceneEntry>& scenes, size_t& activeSceneIdx);
    void endFrame();

private:
    char _filenameBuffer[256] = "output.png";
};