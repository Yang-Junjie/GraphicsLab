#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera3D.hpp"
#include "Scene/Scene.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

class AdvanceLighting : public Scene {
public:
    AdvanceLighting();
    ~AdvanceLighting() override = default;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;
    void OnEvent(flux::Event& event) override;

private:
    GLuint LoadTexture(const char* path);
    void RenderScene(gl::Shader& shader);

    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    // OpenGL resources
    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::Shader> lamp_shader_;
    std::unique_ptr<gl::Shader> depth_shader_;
    std::unique_ptr<gl::VertexArray> floor_vao_;
    std::unique_ptr<gl::VertexArray> cube_vao_;
    std::unique_ptr<gl::VertexArray> scene_cube_vao_;
    std::unique_ptr<gl::Buffer> floor_vbo_;
    std::unique_ptr<gl::Buffer> cube_vbo_;
    std::unique_ptr<gl::Buffer> scene_cube_vbo_;
    GLuint floor_texture_ = 0;
    GLuint depth_map_fbo_ = 0;
    GLuint depth_map_texture_ = 0;

    // shadow mapping parameters
    const unsigned int shadow_width_ = 1'024;
    const unsigned int shadow_height_ = 1'024;

    // Camera
    std::unique_ptr<Camera3D> camera_3d_;

    // Blinn-Phong parameters
    bool use_blinn_phong_ = true;
    // 0: None, 1: Gamma Correction with framebuffer sRGB, 2: Manual gamma correction in shader
    int selected_gamma_correction_ = 0;
    float shininess_ = 32.0f;
    float ambient_strength_ = 0.1f;
    float specular_strength_ = 0.5f;
    glm::vec3 light_pos_ = glm::vec3(-2.0f, 4.0f, -1.0f);
    glm::vec3 light_color_ = glm::vec3(1.0f);

    // Shadow mapping parameters
    bool enable_shadows_ = true;
    int pcf_region_size_ = 3; // PCF kernel size (must be odd)

    // Input state
    std::unordered_set<int> keys_down_;
    bool right_mouse_down_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool first_mouse_ = true;

    // Camera controls
    glm::vec3 camera_pos_ = glm::vec3(0.0f, 1.5f, 4.0f);
    float camera_pitch_ = -20.0f;
    float camera_yaw_ = -90.0f;
    float camera_fov_ = 45.0f;
    float camera_move_speed_ = 5.0f;
    float camera_mouse_sensitivity_ = 0.1f;
};
