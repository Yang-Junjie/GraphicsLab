#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera3D.hpp"

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

namespace flux {
class Event;
}

struct AdvanceLightingRenderResources {
    std::unique_ptr<gl::Shader> shader;
    std::unique_ptr<gl::Shader> lamp_shader;
    std::unique_ptr<gl::Shader> depth_shader;
    std::unique_ptr<gl::Shader> bloom_blur_shader;
    std::unique_ptr<gl::Shader> bloom_composite_shader;

    std::unique_ptr<gl::VertexArray> floor_vao;
    std::unique_ptr<gl::VertexArray> cube_vao;
    std::unique_ptr<gl::VertexArray> scene_cube_vao;
    std::unique_ptr<gl::VertexArray> quad_vao;

    std::unique_ptr<gl::Buffer> floor_vbo;
    std::unique_ptr<gl::Buffer> cube_vbo;
    std::unique_ptr<gl::Buffer> scene_cube_vbo;
    std::unique_ptr<gl::Buffer> quad_vbo;

    GLuint floor_texture = 0;
    GLuint depth_map_fbo = 0;
    GLuint depth_map_texture = 0;

    GLuint bloom_fbo = 0;
    std::array<GLuint, 2> bloom_color_textures = {};
    GLuint bloom_rbo = 0;
    std::array<GLuint, 2> bloom_ping_pong_fbos = {};
    std::array<GLuint, 2> bloom_ping_pong_textures = {};
    int bloom_fbo_width = 0;
    int bloom_fbo_height = 0;
};

struct AdvanceLightingLightingSettings {
    bool use_blinn_phong = true;
    float shininess = 64.0f;
    float ambient_strength = 0.08f;
    float specular_strength = 0.2f;
    glm::vec3 light_pos = glm::vec3(-1.5f, 3.8f, 2.0f);
    glm::vec3 light_color = glm::vec3(1.0f, 0.96f, 0.90f);
    float light_intensity = 2.5f;
};

struct AdvanceLightingShadowSettings {
    unsigned int shadow_width = 1'024;
    unsigned int shadow_height = 1'024;
    bool enable_shadows = true;
    int pcf_region_size = 5;
};

struct AdvanceLightingPostProcessSettings {
    int selected_gamma_correction = 2;
    bool enable_hdr = true;
    float exposure = 1.0f;
    int tone_mapping_mode = 0;
    bool enable_bloom = false;
    float bloom_threshold = 1.2f;
    float bloom_intensity = 0.35f;
    int bloom_blur_iterations = 8;
};

struct AdvanceLightingInputState {
    std::unordered_set<int> keys_down;
    bool right_mouse_down = false;
    float last_mouse_x = 0.0f;
    float last_mouse_y = 0.0f;
    bool first_mouse = true;
};

struct AdvanceLightingCameraState {
    Camera3D camera = Camera3D(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    glm::vec3 position = glm::vec3(0.0f, 1.5f, 4.0f);
    float pitch = -20.0f;
    float yaw = -90.0f;
    float fov = 45.0f;
    float move_speed = 5.0f;
    float mouse_sensitivity = 0.1f;
};

class AdvanceLightingState {
public:
    void OnEnter();
    void OnExit();
    void OnUpdate(float dt);
    void OnRender(float width, float height);
    void OnRenderUI();
    void OnEvent(flux::Event& event);

private:
    void CreateShaders();
    void CreateGeometry();
    void CreateShadowMap();
    void DestroyShadowMap();
    void SyncCamera(float width, float height);
    void EnsureBloomBuffers(int width, int height, bool use_hdr_buffer);

    glm::mat4 BuildLightSpaceMatrix() const;
    void RenderShadowPass(const glm::mat4& light_space_matrix);
    void RenderLightingPass(bool use_hdr_buffer,
                            const std::array<GLint, 4>& viewport,
                            GLint target_framebuffer,
                            const glm::mat4& view,
                            const glm::mat4& projection,
                            const glm::mat4& light_space_matrix);
    void RenderLightCube(const glm::mat4& view, const glm::mat4& projection);
    void RenderPostProcess(const std::array<GLint, 4>& viewport, GLint target_framebuffer);
    void RenderScene(gl::Shader& shader);
    void RenderQuad();
    void CreateBloomFBOs(int width, int height);
    void DestroyBloomFBOs();
    GLuint LoadTexture(const char* path) const;

    int NormalizedPcfRegionSize() const;
    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    AdvanceLightingRenderResources resources_;
    AdvanceLightingLightingSettings lighting_;
    AdvanceLightingShadowSettings shadow_;
    AdvanceLightingPostProcessSettings post_process_;
    AdvanceLightingInputState input_;
    AdvanceLightingCameraState camera_;
};
