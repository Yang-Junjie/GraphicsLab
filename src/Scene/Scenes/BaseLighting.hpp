#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera.hpp"
#include "Camera/Camera2D.hpp"
#include "Camera/Camera3D.hpp"
#include "Scene/Scene.hpp"

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

class BaseLighting : public Scene {
public:
    BaseLighting();
    ~BaseLighting() override = default;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    GLuint LoadTexture(const char* path);
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

    // OpenGL resources
    std::unique_ptr<gl::Shader> object_shader_;
    std::unique_ptr<gl::Shader> lamp_shader_;
    std::unique_ptr<gl::VertexArray> cube_vao_;
    std::unique_ptr<gl::VertexArray> lamp_vao_;
    std::unique_ptr<gl::Buffer> cube_vbo_;
    GLuint diffuse_texture_ = 0;
    GLuint specular_texture_ = 0;

    struct DirectionalLight {
        bool enabled = true;
        glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
        glm::vec3 ambient = glm::vec3(0.05f);
        glm::vec3 diffuse = glm::vec3(0.4f);
        glm::vec3 specular = glm::vec3(0.5f);
    };

    struct PointLight {
        bool enabled = true;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 ambient = glm::vec3(0.05f);
        glm::vec3 diffuse = glm::vec3(0.8f);
        glm::vec3 specular = glm::vec3(1.0f);
        float constant = 1.0f;
        float linear = 0.09f;
        float quadratic = 0.032f;
    };

    struct SpotLight {
        bool enabled = true;
        bool follow_camera = true;
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 ambient = glm::vec3(0.0f);
        glm::vec3 diffuse = glm::vec3(1.0f);
        glm::vec3 specular = glm::vec3(1.0f);
        float constant = 1.0f;
        float linear = 0.09f;
        float quadratic = 0.032f;
        float cut_off = 12.5f;
        float outer_cut_off = 15.0f;
    };

    DirectionalLight dir_light_;
    std::array<PointLight, 4> point_lights_;
    SpotLight spot_light_;
    float material_shininess_ = 32.0f;

    // Input state
    std::unordered_set<int> keys_down_;
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;

    // Camera controls
    float time_ = 0.0f;

    // 3D camera controls
    glm::vec3 camera_3d_pos_ = glm::vec3(0.0f, 2.0f, 5.0f);
    float camera_3d_pitch_ = -20.0f;
    float camera_3d_yaw_ = -90.0f;
    float camera_3d_fov_ = 45.0f;
    float camera_3d_move_speed_ = 5.0f;
    float camera_3d_mouse_sensitivity_ = 0.1f;
};
