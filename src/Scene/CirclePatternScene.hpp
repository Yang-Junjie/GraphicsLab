#pragma once

#include "Scene.hpp"

class CirclePatternScene : public Scene {
public:
    CirclePatternScene();

    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;

private:
    float time_ = 0.0f;
};
