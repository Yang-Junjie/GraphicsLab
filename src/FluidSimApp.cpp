#include "Application.hpp"
#include "Renderer/GL.hpp"
#include "Renderer/Renderer2D.hpp"

#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <memory>

class DemoLayer : public flux::Layer {
public:
    DemoLayer()
        : Layer("DemoLayer")
    {}

    void OnAttach() override
    {
        renderer::Renderer2D::Init("shaders");
        fbo_.Create(viewport_w_, viewport_h_);
    }

    void OnDetach() override
    {
        renderer::Renderer2D::Shutdown();
    }

    void OnUpdate(flux::TimeStep ts) override
    {
        fbo_.Resize(viewport_w_, viewport_h_);
        float w = static_cast<float>(viewport_w_);
        float h = static_cast<float>(viewport_h_);

        fbo_.Bind();
        glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 proj = glm::ortho(0.0f, w, h, 0.0f);
        renderer::Renderer2D::BeginScene(proj);

        // Draw a circle at the center
        renderer::Renderer2D::DrawCircle({w * 0.35f, h * 0.5f}, 60.0f, {0.2f, 0.6f, 0.95f, 1.0f});

        // Draw a quad at the center
        renderer::Renderer2D::DrawQuad(
            {w * 0.65f, h * 0.5f}, {120.0f, 120.0f}, {0.95f, 0.4f, 0.3f, 1.0f});

        renderer::Renderer2D::EndScene();
        fbo_.Unbind();
    }

    void OnRenderUI() override
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        ImVec2 avail = ImGui::GetContentRegionAvail();
        viewport_w_ = std::max(static_cast<int>(avail.x), 1);
        viewport_h_ = std::max(static_cast<int>(avail.y), 1);
        ImGui::Image(static_cast<ImTextureID>(fbo_.ColorTexture()),
                     ImVec2(static_cast<float>(viewport_w_), static_cast<float>(viewport_h_)),
                     ImVec2(0, 1),
                     ImVec2(1, 0));
        ImGui::End();
        ImGui::PopStyleVar();

        auto& stats = renderer::Renderer2D::GetStats();
        ImGui::Begin("Renderer Stats");
        ImGui::Text("Circles:    %u", stats.circle_count);
        ImGui::Text("Quads:      %u", stats.quad_count);
        ImGui::Text("Draw Calls: %u", stats.draw_calls);
        ImGui::End();
    }

    void OnEvent(flux::Event& event) override {}

private:
    gl::Framebuffer fbo_;
    int viewport_w_ = 1'280;
    int viewport_h_ = 720;
};

class FluidSimApp : public flux::Application {
public:
    explicit FluidSimApp(const flux::ApplicationSpecification& spec)
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

    ~FluidSimApp() override = default;
};

std::unique_ptr<flux::Application> flux::CreateApplication()
{
    flux::ApplicationSpecification spec;
    spec.name = "Fluid Simulation";
    spec.width = 1'400;
    spec.height = 900;
    spec.clear_color[0] = 0.08f;
    spec.clear_color[1] = 0.08f;
    spec.clear_color[2] = 0.12f;
    spec.clear_color[3] = 1.0f;
    spec.vsync = false;
    spec.msaa_samples = 4;
    spec.resizable = true;
    spec.imgui_docking_enabled = true;
    spec.imgui_viewports_enabled = true;
    return std::make_unique<FluidSimApp>(spec);
}
