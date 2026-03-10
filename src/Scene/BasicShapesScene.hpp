#pragma once

#include "Scene.hpp"

#include <glad/glad.h>

class BasicShapesScene : public Scene {
public:
    BasicShapesScene();

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;

private:
    GLuint program_ = 0;
    GLuint vao_     = 0;
    GLuint vbo_     = 0;
    GLint  proj_loc_ = -1;
};
