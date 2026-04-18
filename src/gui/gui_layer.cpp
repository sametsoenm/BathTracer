#include "gui_layer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <magic_enum.hpp>

#include "optix/optix_renderer.h"
#include "util/io.h"


GuiLayer::GuiLayer(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

GuiLayer::~GuiLayer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GuiLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiLayer::draw(const types::RenderStatistics& stats, 
    OptixRenderer& renderer, types::RendererType& activeRenderType,
    const std::vector<SceneEntry>& scenes, size_t& activeSceneIdx) {

    ImGui::Begin("BathTracer");


    ImGui::Text("Render time:  %.1f ms", stats.renderTimeMs);
    ImGui::Text("Display time: %.1f ms", stats.displayTimeMs);

    ImGui::Separator();

    ImGui::Text("Samples: %zu", renderer.sampleIndex());

    if (ImGui::Button("Reset accumulation")) {
        renderer.resetAccumulation();
    }

    if (const char* text = renderer.isRendering() ? "Pause render" : "Continue render";
        ImGui::Button(text)) {
        renderer.setIsRendering(!renderer.isRendering());
    }

    ImGui::Separator();

    ImGui::Text("Save Image");
    ImGui::InputText("Filename", _filenameBuffer, sizeof(_filenameBuffer));
    if (ImGui::Button("Save"))
        io::output_image(_filenameBuffer, renderer.buffer().data(), 
            settings::IMAGE_WIDTH, settings::IMAGE_HEIGHT);

    ImGui::Separator();

    ImGui::SetNextItemWidth(100.0f);
    if (auto type = renderer.getType();
        ImGui::BeginCombo("Renderers", types::to_string(type).data())) {
        for (auto t : magic_enum::enum_values<types::RendererType>()) {
            bool selected = t == type;
            if (ImGui::Selectable(types::to_string(t).data(), selected)) {
                activeRenderType = t;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(100.0f);
    if (size_t current = activeSceneIdx;
        ImGui::BeginCombo("Scenes", scenes[current].name.c_str())) {
        for (size_t i = 0; i < scenes.size(); i++) {
            bool selected = scenes[i].name == scenes[current].name;
            if (ImGui::Selectable(scenes[i].name.c_str(), selected)) {
                activeSceneIdx = i;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (bool env = renderer.scene().envmapEnabled();
        ImGui::Checkbox("EnvMap", &env)) {
        renderer.scene().setEnvmapEnabled(env);
        renderer.resetAccumulation();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (size_t current = light::getEnvmapIdx(renderer.scene().environment().name);
        ImGui::BeginCombo("maps", light::envMaps[current].name.c_str())) {
        for (size_t i = 0; i < light::envMaps.size(); i++) {
            bool selected = i == current;
            if (ImGui::Selectable(light::envMaps[i].name.c_str(), selected)) {
                renderer.scene().loadEnvMap(light::envMaps[i].name);
                renderer.reloadEnvironment();
                renderer.resetAccumulation();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    ImGui::Text("Camera");
    if (float blur = renderer.cam().blurRadius();
        ImGui::SliderFloat("Camera Blur", &blur, 0.0f, 0.5f)) {
        renderer.cam().setBlurRadius(blur);
        renderer.resetAccumulation();
    }
    if (float dist = renderer.cam().focusDist();
        ImGui::SliderFloat("Focus distance", &dist, 0.5f, 36.0f, "%.3f",
            ImGuiSliderFlags_Logarithmic)) {
        renderer.cam().setFocusDist(dist);
        renderer.resetAccumulation();
    }
    if (float fov = renderer.cam().fov();
        ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f)) {
        renderer.cam().setFOV(fov);
        renderer.resetAccumulation();
    }

    ImGui::Separator();

    if (bool fog = renderer.scene().fogEnabled();
        ImGui::Checkbox("Fog", &fog)) {
        renderer.scene().setFogEnabled(fog);
        renderer.resetAccumulation();
    }

    if (auto* pbr = dynamic_cast<OptixRenderer*>(&renderer)) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        if (auto mode = pbr->fogMode();
            ImGui::BeginCombo("Fog Mode", to_string(mode).data())) {
            for (auto m : magic_enum::enum_values<FogMode>()) {
                bool selected = m == mode;
                if (ImGui::Selectable(to_string(m).data(), selected)) {
                    pbr->setFogMode(static_cast<FogMode>(m));
                    pbr->resetAccumulation();
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    if (float density = renderer.scene().fogDensity();
        ImGui::SliderFloat("Fog Density", &density, 0.0f, 1.0f)) {
        renderer.scene().setFogDensity(density);
        renderer.resetAccumulation();
    }


    ImGui::End();
}

void GuiLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}