#pragma once

#include "Scene.hpp"

#include "Backend/OpenGL/GL.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class StressTestScene : public Scene {
public:
    StressTestScene();
    ~StressTestScene() override;

    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float dt) override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;

private:
    // GPU particle layout (48 bytes, matches compute/draw shader struct)
    struct GpuParticle {
        glm::vec4 color;
        glm::vec2 position;
        glm::vec2 velocity;
        float size;
        float _pad[3];
    };

    // CPU-mode particle (used with Renderer2D fallback)
    struct Particle {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color;
        float size;
    };

    // --- Mode ---
    bool gpu_mode_ = true;

    // --- CPU-mode data ---
    std::vector<Particle> quads_;
    std::vector<Particle> circles_;

    // --- GPU-mode resources (self-managed) ---
    bool gpu_initialized_ = false;

    gl::VertexArray vao_;
    gl::Buffer vbo_;
    gl::Buffer ebo_;

    GLuint quad_ssbo_ = 0;
    GLuint circle_ssbo_ = 0;

    GLuint compute_program_ = 0;
    gl::Shader quad_draw_shader_;
    gl::Shader circle_draw_shader_;

    GLint cs_dt_loc_ = -1;
    GLint cs_screen_loc_ = -1;
    GLint cs_count_loc_ = -1;

    GLint quad_proj_loc_ = -1;
    GLint quad_time_loc_ = -1;
    GLint circle_proj_loc_ = -1;
    GLint circle_time_loc_ = -1;

    // --- Shared state ---
    int quad_count_ = 10'000;
    int circle_count_ = 10'000;
    bool dirty_ = true;
    float time_ = 0.0f;
    float dt_ = 0.0f;

    float last_width_ = 1280.0f;
    float last_height_ = 720.0f;

    void InitGpuResources();
    void DestroyGpuResources();
    void UploadParticles();
    void RespawnParticles();

    void RenderGpu(float width, float height, const glm::mat4& proj);
    void RenderCpu(float width, float height);

    static GLuint CompileComputeProgram(const std::string& path);
};
