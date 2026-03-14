#pragma once

#include "../Scene.hpp"

#include <glad/glad.h>

class TriangleScene : public Scene {
public:
    TriangleScene();

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;

private:
    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint indirectBuffer_ = 0;
};
