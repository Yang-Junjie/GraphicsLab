#include "Renderer2D.hpp"
#include "GL.hpp"

#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace renderer {

// ---- Embedded Shaders ----

static const char* kQuadVertSrc = R"glsl(
#version 430 core
layout(location = 0) in vec2 a_Pos;

struct QuadInstance {
    vec4 color;
    vec2 position;
    vec2 size;
    float rotation;
    float _pad[3];
};

layout(std430, binding = 0) readonly buffer QuadSSBO {
    QuadInstance instances[];
};

uniform mat4 u_Proj;
out vec4 v_Color;

void main() {
    QuadInstance q = instances[gl_InstanceID];
    float c = cos(q.rotation);
    float s = sin(q.rotation);
    vec2 scaled = a_Pos * q.size;
    vec2 rotated = vec2(scaled.x * c - scaled.y * s,
                        scaled.x * s + scaled.y * c);
    gl_Position = u_Proj * vec4(rotated + q.position, 0.0, 1.0);
    v_Color = q.color;
}
)glsl";

static const char* kQuadFragSrc = R"glsl(
#version 430 core
in vec4 v_Color;
out vec4 FragColor;

void main() {
    FragColor = v_Color;
}
)glsl";

static const char* kCircleVertSrc = R"glsl(
#version 430 core
layout(location = 0) in vec2 a_Pos;

struct CircleInstance {
    vec4 color;
    vec2 position;
    float radius;
    float _pad;
};

layout(std430, binding = 1) readonly buffer CircleSSBO {
    CircleInstance instances[];
};

uniform mat4 u_Proj;
out vec2 v_Local;
out vec4 v_Color;

void main() {
    CircleInstance ci = instances[gl_InstanceID];
    vec2 world = a_Pos * ci.radius * 2.0 + ci.position;
    gl_Position = u_Proj * vec4(world, 0.0, 1.0);
    v_Local = a_Pos * 2.0;
    v_Color = ci.color;
}
)glsl";

static const char* kCircleFragSrc = R"glsl(
#version 430 core
in vec2 v_Local;
in vec4 v_Color;
out vec4 FragColor;

void main() {
    float dist = dot(v_Local, v_Local);
    if (dist > 1.0) discard;
    float edge = fwidth(sqrt(dist));
    float alpha = 1.0 - smoothstep(1.0 - edge * 1.5, 1.0, sqrt(dist));
    FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
}
)glsl";

// ---- Internal State ----

struct RendererState {
    gl::VertexArray vao;
    gl::Buffer vbo;
    gl::Buffer ebo;

    gl::Shader quad_shader;
    gl::Shader circle_shader;

    gl::Buffer quad_ssbo;
    gl::Buffer circle_ssbo;

    GLint quad_proj_loc = -1;
    GLint circle_proj_loc = -1;

    std::vector<QuadInstance> quad_buf;
    std::vector<CircleInstance> circle_buf;

    uint32_t max_quads = 0;
    uint32_t max_circles = 0;

    glm::mat4 projection{1.0f};
    Renderer2DStats stats;
    bool initialized = false;
};

static RendererState* s_state = nullptr;

// ---- Implementation ----

void Renderer2D::Init(uint32_t max_quads, uint32_t max_circles) {
    s_state = new RendererState();
    s_state->max_quads = max_quads;
    s_state->max_circles = max_circles;
    s_state->quad_buf.reserve(max_quads);
    s_state->circle_buf.reserve(max_circles);

    // Unit quad: centered at origin, half-extent = 0.5
    float verts[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };
    uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

    s_state->vao.Bind();
    s_state->vbo.Upload(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    s_state->ebo.Upload(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    s_state->vao.Unbind();

    // Pre-allocate SSBOs
    s_state->quad_ssbo.Upload(GL_SHADER_STORAGE_BUFFER,
        max_quads * sizeof(QuadInstance), nullptr, GL_STREAM_DRAW);
    s_state->circle_ssbo.Upload(GL_SHADER_STORAGE_BUFFER,
        max_circles * sizeof(CircleInstance), nullptr, GL_STREAM_DRAW);

    // Compile shaders
    s_state->quad_shader.Compile(kQuadVertSrc, kQuadFragSrc);
    s_state->circle_shader.Compile(kCircleVertSrc, kCircleFragSrc);

    s_state->quad_proj_loc = s_state->quad_shader.Uniform("u_Proj");
    s_state->circle_proj_loc = s_state->circle_shader.Uniform("u_Proj");

    s_state->initialized = true;
}

void Renderer2D::Shutdown() {
    delete s_state;
    s_state = nullptr;
}

void Renderer2D::BeginScene(const glm::mat4& projection) {
    s_state->projection = projection;
    s_state->quad_buf.clear();
    s_state->circle_buf.clear();
    s_state->stats = {};
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size,
                           float rotation, const glm::vec4& color) {
    s_state->quad_buf.push_back({color, position, size, rotation, {}});
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size,
                           const glm::vec4& color) {
    s_state->quad_buf.push_back({color, position, size, 0.0f, {}});
}

void Renderer2D::DrawCircle(const glm::vec2& position, float radius,
                             const glm::vec4& color) {
    s_state->circle_buf.push_back({color, position, radius, 0.0f});
}

static void FlushQuads() {
    if (s_state->quad_buf.empty()) return;

    auto count = static_cast<GLsizei>(s_state->quad_buf.size());
    GLsizeiptr data_size = count * sizeof(QuadInstance);

    // Upload via buffer orphaning
    s_state->quad_ssbo.Bind(GL_SHADER_STORAGE_BUFFER);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data_size, nullptr, GL_STREAM_DRAW);
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data_size,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(ptr, s_state->quad_buf.data(), data_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    s_state->quad_ssbo.BindBase(GL_SHADER_STORAGE_BUFFER, 0);

    s_state->quad_shader.Bind();
    glUniformMatrix4fv(s_state->quad_proj_loc, 1, GL_FALSE,
                       glm::value_ptr(s_state->projection));

    s_state->vao.Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);

    s_state->stats.quad_count += count;
    s_state->stats.draw_calls++;
}

static void FlushCircles() {
    if (s_state->circle_buf.empty()) return;

    auto count = static_cast<GLsizei>(s_state->circle_buf.size());
    GLsizeiptr data_size = count * sizeof(CircleInstance);

    s_state->circle_ssbo.Bind(GL_SHADER_STORAGE_BUFFER);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data_size, nullptr, GL_STREAM_DRAW);
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data_size,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(ptr, s_state->circle_buf.data(), data_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    s_state->circle_ssbo.BindBase(GL_SHADER_STORAGE_BUFFER, 1);

    s_state->circle_shader.Bind();
    glUniformMatrix4fv(s_state->circle_proj_loc, 1, GL_FALSE,
                       glm::value_ptr(s_state->projection));

    s_state->vao.Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);

    s_state->stats.circle_count += count;
    s_state->stats.draw_calls++;
}

void Renderer2D::EndScene() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    FlushQuads();
    FlushCircles();

    glBindVertexArray(0);
    glUseProgram(0);
}

const Renderer2DStats& Renderer2D::GetStats() {
    return s_state->stats;
}

void Renderer2D::ResetStats() {
    s_state->stats = {};
}

} // namespace renderer
