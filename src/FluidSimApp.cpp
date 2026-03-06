#include <memory>
#include <vector>
#include <cmath>

#include <imgui.h>
#include <glad/glad.h>

#include "Application.hpp"


class FluidLayer : public flux::Layer {
public:
    FluidLayer() : Layer("FluidLayer") {}

    void OnAttach() override {
       
    }

    void OnUpdate(flux::TimeStep ts) override {
       
    }

    void OnRenderUI() override {
      
    }

    void OnEvent(flux::Event& event) override {
        flux::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<flux::KeyPressedEvent>(
            [this](flux::KeyPressedEvent& e) {
                
            });
    }

};

class FluidSimApp : public flux::Application {
public:
    explicit FluidSimApp(const flux::ApplicationSpecification& spec)
        : flux::Application(spec) {
        PushLayer(std::make_unique<FluidLayer>());
        SetMenubarCallback([this]() {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    Close();
                }
                ImGui::EndMenu();
            }
        });
    }

    ~FluidSimApp() override = default;
};

std::unique_ptr<flux::Application> flux::CreateApplication() {
    flux::ApplicationSpecification spec;
    spec.name = "Fluid Simulation";
    spec.width = 1400;
    spec.height = 900;
    spec.clear_color[0] = 0.08f;
    spec.clear_color[1] = 0.08f;
    spec.clear_color[2] = 0.12f;
    spec.clear_color[3] = 1.0f;
    spec.vsync = true;
    spec.msaa_samples = 4;
    spec.resizable = true;
    spec.imgui_docking_enabled = true;
    spec.imgui_viewports_enabled = true;
    return std::make_unique<FluidSimApp>(spec);
}
