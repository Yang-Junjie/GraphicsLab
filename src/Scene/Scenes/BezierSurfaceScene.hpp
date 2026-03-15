#pragma once

#include "../Scene.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Camera/Camera3D.hpp"

#include <array>
#include <glad/glad.h>
#include <glm/glm.hpp>

class BezierSurfaceScene : public Scene {
public:
    BezierSurfaceScene();

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    void ResetControlPoints();
    void BuildCageIndices();
    void UploadControlPoints();

    // Shaders
    gl::Shader surface_shader_;
    gl::Shader cage_shader_;

    // Surface (16 control points as GL_PATCHES)
    GLuint surface_vao_ = 0;
    GLuint surface_vbo_ = 0;

    // Control cage (GL_LINES with index buffer)
    GLuint cage_vao_ = 0;
    GLuint cage_vbo_ = 0;
    GLuint cage_ebo_ = 0;

    // 4x4 control points
    std::array<glm::vec3, 16> control_points_;

    // UI parameters
    float tess_level_ = 16.0f;
    float surface_color_[3] = {0.2f, 0.6f, 1.0f};
    bool wireframe_ = false;
    bool show_cage_ = true;
    bool show_points_ = true;
    bool show_surface_ = true;
    float point_size_ = 8.0f;
    glm::vec3 light_pos_ = glm::vec3(2.0f, 3.0f, 2.0f);
    float shininess_ = 64.0f;

    // Camera (orbit mode)
    Camera3D camera_;
    float rot_x_ = 30.0f;
    float rot_y_ = 45.0f;
    float cam_dist_ = 5.0f;
    float mouse_sensitivity_ = 0.3f;

    // Mouse state
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;
};
