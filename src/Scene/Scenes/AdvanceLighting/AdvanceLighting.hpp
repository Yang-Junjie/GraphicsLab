#pragma once

#include "Scene/Scene.hpp"

#include <memory>

class AdvanceLightingState;

class AdvanceLighting : public Scene {
public:
    AdvanceLighting();
    ~AdvanceLighting() override;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    std::unique_ptr<AdvanceLightingState> state_;
};
