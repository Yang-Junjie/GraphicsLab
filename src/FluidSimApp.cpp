#include <memory>
#include <algorithm>

#include <imgui.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Application.hpp"
#include "Renderer/GL.hpp"
#include "Renderer/Renderer2D.hpp"
#include "Simulation/ParticleSystem.hpp"

class FluidLayer : public flux::Layer {
public:
    FluidLayer() : Layer("FluidLayer") {}

    void OnAttach() override {
        renderer::Renderer2D::Init();
        fbo_.Create(viewport_w_, viewport_h_);
        system_.Init(particle_count_, particle_radius_,
                     static_cast<float>(viewport_w_),
                     static_cast<float>(viewport_h_));
    }

    void OnDetach() override {
        renderer::Renderer2D::Shutdown();
    }

    void OnUpdate(flux::TimeStep ts) override {
        fbo_.Resize(viewport_w_, viewport_h_);
        float w = static_cast<float>(viewport_w_);
        float h = static_cast<float>(viewport_h_);

        system_.Update(ts.GetSeconds(), w, h);

        // Render to FBO
        fbo_.Bind();
        glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 proj = glm::ortho(0.0f, w, h, 0.0f);
        renderer::Renderer2D::BeginScene(proj);

        float radius = system_.GetRadius();
        for (const auto& p : system_.GetParticles()) {
            float speed = glm::length(p.vel);
            float t = std::min(speed / 500.0f, 1.0f);
            glm::vec4 color(
                0.1f + 0.5f * t,
                0.4f + 0.5f * t,
                0.95f,
                0.9f
            );
            renderer::Renderer2D::DrawCircle(p.pos, radius, color);
        }

        // Boundary walls
        glm::vec4 bc(0.35f, 0.35f, 0.4f, 1.0f);
        renderer::Renderer2D::DrawQuad({w / 2, 1.5f},     {w, 3.0f}, bc);
        renderer::Renderer2D::DrawQuad({w / 2, h - 1.5f}, {w, 3.0f}, bc);
        renderer::Renderer2D::DrawQuad({1.5f, h / 2},     {3.0f, h}, bc);
        renderer::Renderer2D::DrawQuad({w - 1.5f, h / 2}, {3.0f, h}, bc);

        renderer::Renderer2D::EndScene();
        fbo_.Unbind();
    }

    void OnRenderUI() override {
        // Viewport
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        ImVec2 avail = ImGui::GetContentRegionAvail();
        viewport_w_ = std::max(static_cast<int>(avail.x), 1);
        viewport_h_ = std::max(static_cast<int>(avail.y), 1);
        ImGui::Image(static_cast<ImTextureID>(fbo_.ColorTexture()),
                     ImVec2(static_cast<float>(viewport_w_),
                            static_cast<float>(viewport_h_)),
                     ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
        ImGui::PopStyleVar();

        // Simulation controls
        ImGui::Begin("Simulation");
        ImGui::SliderFloat("Gravity",     &system_.gravity,     0.0f, 2000.0f);
        ImGui::SliderFloat("Restitution", &system_.restitution, 0.0f, 1.0f);
        ImGui::SliderFloat("Damping",     &system_.damping,     0.95f, 1.0f);
        ImGui::SliderInt("Solver Iters",  &system_.solver_iterations, 1, 8);
        ImGui::SliderInt("Sub Steps",     &system_.sub_steps,   1, 8);
        ImGui::Separator();
        ImGui::InputInt("Particle Count", &particle_count_);
        particle_count_ = std::clamp(particle_count_, 100, 500000);
        ImGui::SliderFloat("Radius", &particle_radius_, 1.0f, 8.0f);
        if (ImGui::Button("Reset")) {
            system_.Init(particle_count_, particle_radius_,
                         static_cast<float>(viewport_w_),
                         static_cast<float>(viewport_h_));
        }
        ImGui::End();

        // Renderer stats
        auto& stats = renderer::Renderer2D::GetStats();
        ImGui::Begin("Renderer Stats");
        ImGui::Text("Particles:  %d",  system_.GetCount());
        ImGui::Text("Circles:    %u",  stats.circle_count);
        ImGui::Text("Quads:      %u",  stats.quad_count);
        ImGui::Text("Draw Calls: %u",  stats.draw_calls);
        ImGui::End();
    }

    void OnEvent(flux::Event& event) override {}

private:
    sim::ParticleSystem system_;
    gl::Framebuffer fbo_;
    int viewport_w_ = 1280;
    int viewport_h_ = 720;
    int particle_count_ = 1000;
    float particle_radius_ = 10.0f;
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
    spec.vsync = false;
    spec.msaa_samples = 4;
    spec.resizable = true;
    spec.imgui_docking_enabled = true;
    spec.imgui_viewports_enabled = true;
    return std::make_unique<FluidSimApp>(spec);
}
