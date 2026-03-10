#pragma once

#include "Scene.hpp"

class GridScene : public Scene {
public:
    GridScene();

    void OnRender(float width, float height) override;
};
