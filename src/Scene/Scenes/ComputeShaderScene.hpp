#pragma once

#include "../Scene.hpp"
#include "Backend/OpenGL/Shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

class ComputeShaderScene : public Scene {
public:
    ComputeShaderScene();

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;

private:
    gl::Shader compute_shader_;

    GLuint ssbo_ = 0;

    static constexpr int N_ = 1'024;
    float data_[N_] = {};
    bool ready_ = false;
};
