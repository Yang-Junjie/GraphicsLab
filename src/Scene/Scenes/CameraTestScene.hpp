#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera.hpp"
#include "Camera/Camera2D.hpp"
#include "Camera/Camera3D.hpp"
#include "Scene/Scene.hpp"

#include <memory>
#include <unordered_set>

enum class CameraMode {
    None,
    Ortho2D,
    Perspective3D,
    Orthographic3D
};

class CameraTestScene : public Scene {
public:
    CameraTestScene();
    ~CameraTestScene() override = default;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    void SetupCamera(CameraMode mode, float width, float height);

    // Input handlers
    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    // Camera instances
    std::unique_ptr<Camera2D> camera_2d_;
    std::unique_ptr<Camera3D> camera_3d_;
    Camera* active_camera_ = nullptr;
    CameraMode camera_mode_ = CameraMode::None;

    // OpenGL resources
    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::VertexArray> quad_vao_;
    std::unique_ptr<gl::Buffer> quad_vbo_;
    GLuint texture_ = 0;

    // Input state
    std::unordered_set<int> keys_down_;
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;

    // Camera controls
    float time_ = 0.0f;

    // 2D camera controls
    glm::vec2 camera_2d_pos_ = glm::vec2(0.0f);
    float camera_2d_rotation_ = 0.0f;
    float camera_2d_zoom_ = 1.0f;
    float camera_2d_move_speed_ = 5.0f;
    float camera_2d_rotate_speed_ = 90.0f;

    // 3D camera controls
    glm::vec3 camera_3d_pos_ = glm::vec3(0.0f, 2.0f, 5.0f);
    float camera_3d_pitch_ = -20.0f;
    float camera_3d_yaw_ = -90.0f;
    float camera_3d_fov_ = 45.0f;
    float camera_3d_move_speed_ = 5.0f;
    float camera_3d_mouse_sensitivity_ = 0.1f;
};
