#pragma once

#include "Scene/Scene.hpp"

#include <memory>

namespace flux {
class Event;
}

class PBRState;

class PBR : public Scene {
public:
    PBR();
    ~PBR() override;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    std::unique_ptr<PBRState> state_;
};
