#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera3D.hpp"
#include "Scene/Scene.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_set>

class AdvancedOpenGL : public Scene {
public:
    AdvancedOpenGL();
    ~AdvancedOpenGL() override = default;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    GLuint LoadTexture(const char* path, bool flip_vertical = true) const;

    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::Shader> outline_shader_;
    std::unique_ptr<gl::VertexArray> cube_vao_;
    std::unique_ptr<gl::VertexArray> plane_vao_;
    std::unique_ptr<gl::Buffer> cube_vbo_;
    std::unique_ptr<gl::Buffer> plane_vbo_;
    GLuint cube_texture_ = 0;
    GLuint floor_texture_ = 0;

    std::unique_ptr<Camera3D> camera_;
    std::string status_text_ = "Ready";

    bool depth_test_enabled_ = true;
    int depth_func_index_ = 0;
    bool outline_enabled_ = true;
    float outline_scale_ = 1.05f;
    float outline_color_[3] = {0.04f, 0.28f, 0.26f};

    std::unordered_set<int> keys_down_;
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;

    glm::vec3 camera_3d_pos_ = glm::vec3(0.0f, 0.0f, 3.0f);
    float camera_3d_pitch_ = 0.0f;
    float camera_3d_yaw_ = -90.0f;
    float camera_3d_fov_ = 45.0f;
    float camera_3d_move_speed_ = 4.0f;
    float camera_3d_mouse_sensitivity_ = 0.1f;
};
