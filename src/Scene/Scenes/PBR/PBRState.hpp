#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/Shader.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Camera/Camera3D.hpp"
#include "Model/Model.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

#include <glm/glm.hpp>

namespace flux {
class Event;
}

struct PBRRenderResources {
    std::unique_ptr<gl::Shader> geometry_shader;
    std::unique_ptr<gl::Shader> lighting_shader;
    std::unique_ptr<gl::Shader> equirect_to_cubemap_shader;
    std::unique_ptr<gl::Shader> irradiance_shader;
    std::unique_ptr<gl::Shader> prefilter_shader;
    std::unique_ptr<gl::Shader> brdf_lut_shader;

    std::unique_ptr<gl::VertexArray> quad_vao;
    std::unique_ptr<gl::VertexArray> cube_vao;
    std::unique_ptr<Model> model;

    std::unique_ptr<gl::Buffer> quad_vbo;
    std::unique_ptr<gl::Buffer> cube_vbo;

    GLuint g_buffer_fbo = 0;
    GLuint g_position_texture = 0;
    GLuint g_normal_texture = 0;
    GLuint g_albedo_texture = 0;
    GLuint g_metallic_texture = 0;
    GLuint g_roughness_texture = 0;
    GLuint g_ao_texture = 0;
    GLuint g_emissive_texture = 0;
    GLuint g_depth_rbo = 0;
    int g_buffer_width = 0;
    int g_buffer_height = 0;

    GLuint capture_fbo = 0;
    GLuint capture_rbo = 0;
    GLuint hdr_texture = 0;
    GLuint environment_cubemap = 0;
    GLuint irradiance_cubemap = 0;
    GLuint prefilter_cubemap = 0;
    GLuint brdf_lut_texture = 0;
    int prefilter_mip_levels = 0;
};

struct PBRMaterialSettings {
    float model_scale = 1.0f;
};

struct PBRPointLight {
    glm::vec3 position = glm::vec3(2.5f, 2.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.90f);
    float intensity = 35.0f;
};

struct PBRLightingSettings {
    PBRPointLight light;
    float ambient_intensity = 0.03f;
    float ibl_intensity = 1.0f;
    float environment_intensity = 1.0f;
    float exposure = 1.0f;
    float background_lod = 0.0f;
    glm::vec3 background_color = glm::vec3(0.03f, 0.04f, 0.06f);
    bool show_environment = true;
};

struct PBRInputState {
    std::unordered_set<int> keys_down;
    bool right_mouse_down = false;
    float last_mouse_x = 0.0f;
    float last_mouse_y = 0.0f;
    bool first_mouse = true;
};

struct PBRCameraState {
    Camera3D camera = Camera3D(45.0f, 4.0f / 3.0f, 0.01f, 20.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.4f, 0.45f);
    float pitch = -45.0f;
    float yaw = -90.0f;
    float fov = 45.0f;
    float move_speed = 2.0f;
    float mouse_sensitivity = 0.1f;
};

struct PBRDebugState {
    int selected_gbuffer_preview = 0;
    int lighting_output_mode = 0;
};

class PBRState {
public:
    explicit PBRState(std::filesystem::path model_path = {});

    void OnEnter();
    void OnExit();
    void OnUpdate(float dt);
    void OnRender(float width, float height);
    void OnRenderUI();
    void OnEvent(flux::Event& event);

private:
    bool CreateShaders();
    void CreateGeometry();
    void CreateCubeGeometry();
    bool LoadModel();
    bool CreateImageBasedLighting();
    void CreateQuadGeometry();
    void CreateGBuffer(int width, int height);
    void DestroyGBuffer();
    void DestroyImageBasedLighting();
    void EnsureGBuffer(int width, int height);
    void SyncCamera(float width, float height);
    void RenderGeometryPass(int width,
                            int height,
                            const glm::mat4& view,
                            const glm::mat4& projection);
    void RenderLightingPass(const std::array<GLint, 4>& viewport,
                            GLint target_framebuffer,
                            const glm::mat4& view,
                            const glm::mat4& projection);
    void RenderQuad();
    void RenderCube();

    bool OnKeyPressed(int keycode);
    bool OnKeyReleased(int keycode);
    bool OnMouseButtonPressed(int button);
    bool OnMouseButtonReleased(int button);
    bool OnMouseMoved(float x, float y);
    bool OnMouseScrolled(float x_offset, float y_offset);

    PBRRenderResources resources_;
    PBRMaterialSettings material_;
    PBRLightingSettings lighting_;
    PBRInputState input_;
    PBRCameraState camera_;
    PBRDebugState debug_;
    std::filesystem::path configured_model_path_;
    std::filesystem::path model_path_;
    std::filesystem::path hdr_path_;
    std::string model_status_;
    std::string hdr_status_;
    bool ibl_ready_ = false;
    bool ready_ = false;
};
