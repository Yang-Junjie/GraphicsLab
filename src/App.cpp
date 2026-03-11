#include "App.hpp"
#include "DemoLayer.hpp"

#include <imgui.h>
#include <memory>

FluidSimApp::FluidSimApp(const flux::ApplicationSpecification& spec)
    : flux::Application(spec)
{
    PushLayer(std::make_unique<DemoLayer>());
    SetMenubarCallback([this]() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                Close();
            }
            ImGui::EndMenu();
        }
    });
}

std::unique_ptr<flux::Application> flux::CreateApplication()
{
    flux::ApplicationSpecification spec;
    spec.window.title = "CGDemo";
    spec.window.width = 1'400;
    spec.window.height = 900;
    spec.clear_color[0] = 0.08f;
    spec.clear_color[1] = 0.08f;
    spec.clear_color[2] = 0.12f;
    spec.clear_color[3] = 1.0f;
    spec.window.vsync = false;
    spec.window.msaa_samples = 4;
    spec.window.resizable = true;
    spec.imgui_docking_enabled = true;
    spec.imgui_viewports_enabled = true;
    return std::make_unique<FluidSimApp>(spec);
}
