#pragma once

#include "../Scene.hpp"
#include "Backend/OpenGL/Shader.hpp"

#include <array>
#include <glad/glad.h>
#include <glm/glm.hpp>

class BezierCurveScene : public Scene {
public:
    BezierCurveScene();

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;

private:
    gl::Shader bezier_shader_;
    gl::Shader overlay_shader_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    std::array<glm::vec2, 4> control_points_;

    float tess_level_ = 64.0f;
    float curve_color_[3] = {1.0f, 0.8f, 0.0f};
    float line_width_ = 3.0f;
    bool show_polygon_ = true;
    bool show_points_ = true;
    float point_size_ = 10.0f;
};
