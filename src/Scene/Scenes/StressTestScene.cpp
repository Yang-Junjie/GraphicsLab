#include "Renderer/Renderer2D.hpp"
#include "StressTestScene.hpp"

#include <cmath>

#include <fstream>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <random>
#include <sstream>
#include <thread>

// ---- Helpers ----

template <typename F>
static void ParallelFor(int begin, int end, F&& func)
{
    int n = end - begin;
    if (n <= 0) {
        return;
    }

    int nthreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    int chunk = (n + nthreads - 1) / nthreads;

    std::vector<std::thread> threads;
    threads.reserve(nthreads);

    for (int t = 0; t < nthreads; ++t) {
        int lo = begin + t * chunk;
        int hi = std::min(lo + chunk, end);
        if (lo >= hi) {
            break;
        }
        threads.emplace_back([lo, hi, &func]() {
            for (int i = lo; i < hi; ++i) {
                func(i);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
}

GLuint StressTestScene::CompileComputeProgram(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();
    const char* src = source.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1'024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "Compute shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1'024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        fprintf(stderr, "Compute program link error: %s\n", log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// ---- Lifecycle ----

StressTestScene::StressTestScene()
    : Scene("Stress Test")
{}

StressTestScene::~StressTestScene()
{
    DestroyGpuResources();
}

void StressTestScene::OnEnter()
{
    time_ = 0.0f;
    dirty_ = true;
}

void StressTestScene::OnExit()
{
    // Keep GPU resources alive across scene switches (they persist in the scene object)
}

void StressTestScene::OnUpdate(float dt)
{
    dt_ = dt;
    time_ += dt;

    if (gpu_mode_) {
        // Position update happens on GPU via compute shader in OnRender
        return;
    }

    // CPU mode: parallel position update
    float w = last_width_;
    float h = last_height_;

    ParallelFor(0, static_cast<int>(quads_.size()), [&](int i) {
        auto& p = quads_[i];
        p.position += p.velocity * dt;
        p.position = glm::mod(p.position, glm::vec2(w, h));
    });

    ParallelFor(0, static_cast<int>(circles_.size()), [&](int i) {
        auto& p = circles_[i];
        p.position += p.velocity * dt;
        p.position = glm::mod(p.position, glm::vec2(w, h));
    });
}

// ---- GPU Resources ----

void StressTestScene::InitGpuResources()
{
    if (gpu_initialized_) {
        return;
    }

    // Unit quad VAO
    float verts[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    vao_.Bind();
    vbo_.Upload(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    ebo_.Upload(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    vao_.Unbind();

    // Compute shader
    compute_program_ = CompileComputeProgram("shaders/StressTestScene/particle_update.comp");
    if (compute_program_) {
        cs_dt_loc_ = glGetUniformLocation(compute_program_, "u_DeltaTime");
        cs_screen_loc_ = glGetUniformLocation(compute_program_, "u_ScreenSize");
        cs_count_loc_ = glGetUniformLocation(compute_program_, "u_Count");
    }

    // Draw shaders (reuse existing circle.frag / quad.frag)
    quad_draw_shader_.CompileFromFile("shaders/StressTestScene/stress_quad.vert", "shaders/quad.frag");
    quad_proj_loc_ = quad_draw_shader_.Uniform("u_Proj");
    quad_time_loc_ = quad_draw_shader_.Uniform("u_Time");

    circle_draw_shader_.CompileFromFile("shaders/StressTestScene/stress_circle.vert", "shaders/circle.frag");
    circle_proj_loc_ = circle_draw_shader_.Uniform("u_Proj");
    circle_time_loc_ = circle_draw_shader_.Uniform("u_Time");

    gpu_initialized_ = true;
}

void StressTestScene::DestroyGpuResources()
{
    if (quad_ssbo_) {
        glDeleteBuffers(1, &quad_ssbo_);
        quad_ssbo_ = 0;
    }
    if (circle_ssbo_) {
        glDeleteBuffers(1, &circle_ssbo_);
        circle_ssbo_ = 0;
    }
    if (compute_program_) {
        glDeleteProgram(compute_program_);
        compute_program_ = 0;
    }
    gpu_initialized_ = false;
}

void StressTestScene::UploadParticles()
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_x(0.0f, last_width_);
    std::uniform_real_distribution<float> dist_y(0.0f, last_height_);
    std::uniform_real_distribution<float> dist_vel(-80.0f, 80.0f);
    std::uniform_real_distribution<float> dist_size(2.0f, 8.0f);
    std::uniform_real_distribution<float> dist_color(0.2f, 1.0f);

    // Generate quad particles
    std::vector<GpuParticle> gpu_quads(static_cast<size_t>(quad_count_));
    for (auto& p : gpu_quads) {
        p.color = {dist_color(rng), dist_color(rng), dist_color(rng), 0.8f};
        p.position = {dist_x(rng), dist_y(rng)};
        p.velocity = {dist_vel(rng), dist_vel(rng)};
        p.size = dist_size(rng);
        p._pad[0] = p._pad[1] = p._pad[2] = 0.0f;
    }

    // Generate circle particles
    std::vector<GpuParticle> gpu_circles(static_cast<size_t>(circle_count_));
    for (auto& p : gpu_circles) {
        p.color = {dist_color(rng), dist_color(rng), dist_color(rng), 0.8f};
        p.position = {dist_x(rng), dist_y(rng)};
        p.velocity = {dist_vel(rng), dist_vel(rng)};
        p.size = dist_size(rng);
        p._pad[0] = p._pad[1] = p._pad[2] = 0.0f;
    }

    // Upload quad SSBO
    if (quad_ssbo_) {
        glDeleteBuffers(1, &quad_ssbo_);
    }
    glGenBuffers(1, &quad_ssbo_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, quad_ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(quad_count_) * sizeof(GpuParticle),
                 gpu_quads.data(),
                 GL_DYNAMIC_COPY);

    // Upload circle SSBO
    if (circle_ssbo_) {
        glDeleteBuffers(1, &circle_ssbo_);
    }
    glGenBuffers(1, &circle_ssbo_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, circle_ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(circle_count_) * sizeof(GpuParticle),
                 gpu_circles.data(),
                 GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// ---- CPU-mode particle init (for Renderer2D path) ----

void StressTestScene::RespawnParticles()
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_x(0.0f, last_width_);
    std::uniform_real_distribution<float> dist_y(0.0f, last_height_);
    std::uniform_real_distribution<float> dist_vel(-80.0f, 80.0f);
    std::uniform_real_distribution<float> dist_size(2.0f, 8.0f);
    std::uniform_real_distribution<float> dist_color(0.2f, 1.0f);

    quads_.resize(static_cast<size_t>(quad_count_));
    for (auto& p : quads_) {
        p.position = {dist_x(rng), dist_y(rng)};
        p.velocity = {dist_vel(rng), dist_vel(rng)};
        p.size = dist_size(rng);
        p.color = {dist_color(rng), dist_color(rng), dist_color(rng), 0.8f};
    }

    circles_.resize(static_cast<size_t>(circle_count_));
    for (auto& p : circles_) {
        p.position = {dist_x(rng), dist_y(rng)};
        p.velocity = {dist_vel(rng), dist_vel(rng)};
        p.size = dist_size(rng);
        p.color = {dist_color(rng), dist_color(rng), dist_color(rng), 0.8f};
    }
}

// ---- Rendering ----

void StressTestScene::RenderGpu(float width, float height, const glm::mat4& proj)
{
    if (!compute_program_) {
        return;
    }

    // --- Compute pass: update positions ---
    glUseProgram(compute_program_);
    glUniform1f(cs_dt_loc_, dt_);
    glUniform2f(cs_screen_loc_, width, height);

    if (quad_count_ > 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, quad_ssbo_);
        glUniform1ui(cs_count_loc_, static_cast<GLuint>(quad_count_));
        glDispatchCompute(static_cast<GLuint>((quad_count_ + 255) / 256), 1, 1);
    }

    if (circle_count_ > 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, circle_ssbo_);
        glUniform1ui(cs_count_loc_, static_cast<GLuint>(circle_count_));
        glDispatchCompute(static_cast<GLuint>((circle_count_ + 255) / 256), 1, 1);
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // --- Draw pass ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    vao_.Bind();

    // Draw quads
    if (quad_count_ > 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, quad_ssbo_);
        quad_draw_shader_.Bind();
        glUniformMatrix4fv(quad_proj_loc_, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(quad_time_loc_, time_);
        glDrawElementsInstanced(
            GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(quad_count_));
    }

    // Draw circles
    if (circle_count_ > 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, circle_ssbo_);
        circle_draw_shader_.Bind();
        glUniformMatrix4fv(circle_proj_loc_, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(circle_time_loc_, time_);
        glDrawElementsInstanced(
            GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(circle_count_));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void StressTestScene::RenderCpu(float width, float height)
{
    float t = time_;

    if (!quads_.empty()) {
        auto* dst = renderer::Renderer2D::AllocQuads(static_cast<uint32_t>(quads_.size()));
        ParallelFor(0, static_cast<int>(quads_.size()), [&](int i) {
            const auto& p = quads_[i];
            float r = 0.5f + 0.5f * std::sin(p.position.x * 0.01f + t);
            float g = 0.5f + 0.5f * std::sin(p.position.y * 0.01f + t * 1.3f);
            dst[i] = {{r, g, p.color.b, 0.8f}, p.position, {p.size, p.size}, 0.0f, {}};
        });
    }

    if (!circles_.empty()) {
        auto* dst = renderer::Renderer2D::AllocCircles(static_cast<uint32_t>(circles_.size()));
        ParallelFor(0, static_cast<int>(circles_.size()), [&](int i) {
            const auto& p = circles_[i];
            float g = 0.5f + 0.5f * std::sin(p.position.x * 0.01f + t * 0.7f);
            float b = 0.5f + 0.5f * std::sin(p.position.y * 0.01f + t * 1.1f);
            dst[i] = {{p.color.r, g, b, 0.8f}, p.position, p.size, 0.0f};
        });
    }
}

void StressTestScene::OnRender(float width, float height)
{
    last_width_ = width;
    last_height_ = height;

    if (gpu_mode_) {
        InitGpuResources();
        if (dirty_) {
            UploadParticles();
            dirty_ = false;
        }
        glm::mat4 proj = glm::ortho(0.0f, width, height, 0.0f);
        RenderGpu(width, height, proj);
    } else {
        if (dirty_) {
            RespawnParticles();
            dirty_ = false;
        }
        RenderCpu(width, height);
    }
}

void StressTestScene::OnRenderUI()
{
    // Mode toggle
    bool mode_changed = false;
    if (ImGui::RadioButton("GPU (Compute Shader)", gpu_mode_)) {
        if (!gpu_mode_) {
            gpu_mode_ = true;
            mode_changed = true;
        }
    }

    if (ImGui::RadioButton("CPU (ParallelFor)", !gpu_mode_)) {
        if (gpu_mode_) {
            gpu_mode_ = false;
            mode_changed = true;
        }
    }

    if (mode_changed) {
        dirty_ = true;
    }

    ImGui::Separator();

    bool changed = false;
    changed |= ImGui::SliderInt("Quads", &quad_count_, 0, 10'000'000);
    changed |= ImGui::SliderInt("Circles", &circle_count_, 0, 10'000'000);

    if (changed) {
        dirty_ = true;
    }

    ImGui::Separator();
    ImGui::Text("Mode: %s", gpu_mode_ ? "GPU" : "CPU");
    int total = quad_count_ + circle_count_;
    if (total >= 1'000'000) {
        ImGui::Text("Total: %.1fM particles", total / 1'000'000.0f);
    } else {
        ImGui::Text("Total: %d particles", total);
    }
}
