#pragma once

#include "Backend/OpenGL/Shader.hpp"
#include "Camera/Camera3D.hpp"
#include "Model/Model.hpp"
#include "Scene/Scene.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_set>

class ModelScene : public Scene {
public:
    ModelScene();
    ~ModelScene() override = default;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    // Input handlers
    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<Model> model_;
    std::unique_ptr<Camera3D> camera_;

    bool model_loaded_ = false;
    std::string status_text_;

    bool auto_rotate_ = true;
    float rotation_y_deg_ = 0.0f;
    float rotation_speed_deg_ = 20.0f;
    float model_scale_ = 1.0f;

    std::unordered_set<int> keys_down_;
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;

    glm::vec3 camera_3d_pos_ = glm::vec3(0.0f, 0.4f, 0.45f);
    float camera_3d_pitch_ = -45.0f;
    float camera_3d_yaw_ = -90.0f;
    float camera_3d_fov_ = 45.0f;
    float camera_3d_move_speed_ = 2.0f;
    float camera_3d_mouse_sensitivity_ = 0.1f;

    glm::vec3 light_direction_ = glm::vec3(-0.3f, -1.0f, -0.2f);
    glm::vec3 light_color_ = glm::vec3(1.0f);
    glm::vec3 ambient_color_ = glm::vec3(0.18f, 0.18f, 0.20f);
};
