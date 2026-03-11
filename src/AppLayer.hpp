#pragma once

#include "Backend/OpenGL/Framebuffer.hpp"
#include "Event.hpp"
#include "JobSystem/JobSystem.hpp"
#include "Layer.hpp"
#include "Scene/Scene.hpp"
#include "TimeStep.hpp"

#include <memory>
#include <vector>

class AppLayer : public flux::Layer {
public:
    AppLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(flux::TimeStep ts) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    gl::Framebuffer fbo_;
    int viewport_w_ = 1'280;
    int viewport_h_ = 720;

    std::unique_ptr<job::JobSystem> jobs_;
    job::Counter update_counter_;
    bool first_frame_ = true;

    std::vector<std::unique_ptr<Scene>> scenes_;
    int current_scene_ = 0;

    static constexpr int kFrameSampleCount = 600;
    float frame_samples_[kFrameSampleCount] = {};
    int frame_sample_index_ = 0;
    float frame_time_avg_ms_ = 0.0f;
};
