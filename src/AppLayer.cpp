#include "AppLayer.hpp"
#include "Renderer/Renderer2D.hpp"
#include "Scene/Scenes/StressTestScene.hpp"
#include "Scene/Scenes/TextureScene.hpp"
#include "Scene/Scenes/TriangleScene.hpp"
#include "Scene/Scenes/CameraTestScene.hpp"

#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

AppLayer::AppLayer()
    : Layer("AppLayer")
{}

void AppLayer::OnAttach()
{
    renderer::Renderer2D::Init("shaders");
    fbo_.Create(viewport_w_, viewport_h_);

    jobs_ = std::make_unique<job::JobSystem>();

    scenes_.push_back(std::make_unique<TriangleScene>());
    scenes_.push_back(std::make_unique<StressTestScene>());
    scenes_.push_back(std::make_unique<TextureScene>());
    scenes_.push_back(std::make_unique<CameraTestScene>());

    current_scene_ = 0;
    scenes_[current_scene_]->OnEnter();
}

void AppLayer::OnDetach()
{
    update_counter_.Wait();
    jobs_.reset();

    if (!scenes_.empty()) {
        scenes_[current_scene_]->OnExit();
    }
    scenes_.clear();
    renderer::Renderer2D::Shutdown();
}

void AppLayer::OnUpdate(flux::TimeStep ts)
{
    // ---- frame timing ----
    frame_samples_[frame_sample_index_] = ts.GetMilliseconds();
    frame_sample_index_ = (frame_sample_index_ + 1) % kFrameSampleCount;
    float sum = 0.0f;
    for (int i = 0; i < kFrameSampleCount; ++i) {
        sum += frame_samples_[i];
    }
    frame_time_avg_ms_ = sum / static_cast<float>(kFrameSampleCount);

    // 1. Wait for previous frame's worker to finish
    update_counter_.Wait();

    // 2. GL render – reads scene state (no worker active, safe)
    fbo_.Resize(viewport_w_, viewport_h_);
    float w = static_cast<float>(viewport_w_);
    float h = static_cast<float>(viewport_h_);

    fbo_.Bind();
    glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glm::mat4 proj = glm::ortho(0.0f, w, h, 0.0f);
    renderer::Renderer2D::BeginScene(proj);
    scenes_[current_scene_]->OnRender(w, h);
    renderer::Renderer2D::EndScene();

    fbo_.Unbind();

    // 3. Submit scene update to worker (runs in parallel with ImGui)
    float dt = ts;
    int idx = current_scene_;
    if (first_frame_) {
        scenes_[idx]->OnUpdate(dt);
        first_frame_ = false;
    } else {
        jobs_->Submit(
            [this, idx, dt]() {
                scenes_[idx]->OnUpdate(dt);
            },
            &update_counter_);
    }
}

void AppLayer::OnRenderUI()
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

    // Scene selector
    ImGui::Begin("Scene");
    const char* current_name = scenes_[current_scene_]->GetName().c_str();
    if (ImGui::BeginCombo("##SceneCombo", current_name)) {
        for (int i = 0; i < static_cast<int>(scenes_.size()); ++i) {
            bool selected = (i == current_scene_);
            if (ImGui::Selectable(scenes_[i]->GetName().c_str(), selected)) {
                if (i != current_scene_) {
                    update_counter_.Wait();
                    scenes_[current_scene_]->OnExit();
                    current_scene_ = i;
                    scenes_[current_scene_]->OnEnter();
                    first_frame_ = true;
                }
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    scenes_[current_scene_]->OnRenderUI();
    ImGui::End();

    // stats
    auto& stats = renderer::Renderer2D::GetStats();
    ImGui::Begin("Stats");
    float fps = (frame_time_avg_ms_ > 0.0f) ? 1000.0f / frame_time_avg_ms_ : 0.0f;
    ImGui::Text("FPS:        %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", frame_time_avg_ms_);
    float graph_w = ImGui::GetContentRegionAvail().x;
    ImGui::PlotHistogram("##frametime",
                         frame_samples_,
                         kFrameSampleCount,
                         frame_sample_index_,
                         nullptr,
                         0.0f,
                         33.3f,
                         ImVec2(graph_w, 50));
    ImGui::Separator();

    // Renderer API display
    RendererAPI current_api = scenes_[current_scene_]->GetRendererAPI();
    const char* api_name = Scene::RendererAPIToString(current_api);
    ImGui::Text("Renderer:   %s", api_name);

    // Future: RHI switching (only when RHI is implemented)
    // if (current_api == RendererAPI::RHI) {
    //     if (ImGui::BeginCombo("##APICombo", api_name)) {
    //         // Add API options here
    //         ImGui::EndCombo();
    //     }
    // }

    ImGui::Separator();
    ImGui::Text("Circles:    %u", stats.circle_count);
    ImGui::Text("Quads:      %u", stats.quad_count);
    ImGui::Text("Draw Calls: %u", stats.draw_calls);
    ImGui::Text("GPU Time:   %.3f ms", stats.gpu_time_ms);
    ImGui::Separator();
    ImGui::Text("Workers:    %d", jobs_->ThreadCount());
    ImGui::End();
}

void AppLayer::OnEvent(flux::Event& event)
{
    if (!scenes_.empty()) {
        scenes_[current_scene_]->OnEvent(event);
    }
}
