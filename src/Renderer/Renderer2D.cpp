#include "GL.hpp"
#include "Renderer2D.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace renderer {

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

void Renderer2D::Init(const std::string& shader_dir, uint32_t max_quads, uint32_t max_circles)
{
    s_state = new RendererState();
    s_state->max_quads = max_quads;
    s_state->max_circles = max_circles;
    s_state->quad_buf.reserve(max_quads);
    s_state->circle_buf.reserve(max_circles);

    // Unit quad: centered at origin, half-extent = 0.5
    float verts[] = {
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
    };
    uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    s_state->vao.Bind();
    s_state->vbo.Upload(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    s_state->ebo.Upload(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    s_state->vao.Unbind();

    // Pre-allocate SSBOs
    s_state->quad_ssbo.Upload(
        GL_SHADER_STORAGE_BUFFER, max_quads * sizeof(QuadInstance), nullptr, GL_STREAM_DRAW);
    s_state->circle_ssbo.Upload(
        GL_SHADER_STORAGE_BUFFER, max_circles * sizeof(CircleInstance), nullptr, GL_STREAM_DRAW);

    // Compile shaders from files
    s_state->quad_shader.CompileFromFile(shader_dir + "/quad.vert", shader_dir + "/quad.frag");
    s_state->circle_shader.CompileFromFile(shader_dir + "/circle.vert",
                                           shader_dir + "/circle.frag");

    s_state->quad_proj_loc = s_state->quad_shader.Uniform("u_Proj");
    s_state->circle_proj_loc = s_state->circle_shader.Uniform("u_Proj");

    s_state->initialized = true;
}

void Renderer2D::Shutdown()
{
    delete s_state;
    s_state = nullptr;
}

void Renderer2D::BeginScene(const glm::mat4& projection)
{
    s_state->projection = projection;
    s_state->quad_buf.clear();
    s_state->circle_buf.clear();
    s_state->stats = {};
}

void Renderer2D::DrawQuad(const glm::vec2& position,
                          const glm::vec2& size,
                          float rotation,
                          const glm::vec4& color)
{
    s_state->quad_buf.push_back({color, position, size, rotation, {}});
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    s_state->quad_buf.push_back({color, position, size, 0.0f, {}});
}

void Renderer2D::DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color)
{
    s_state->circle_buf.push_back({color, position, radius, 0.0f});
}

static void FlushQuads()
{
    if (s_state->quad_buf.empty()) {
        return;
    }

    auto count = static_cast<GLsizei>(s_state->quad_buf.size());
    GLsizeiptr data_size = count * sizeof(QuadInstance);

    // Upload via buffer orphaning
    s_state->quad_ssbo.Bind(GL_SHADER_STORAGE_BUFFER);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data_size, nullptr, GL_STREAM_DRAW);
    void* ptr = glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(ptr, s_state->quad_buf.data(), data_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    s_state->quad_ssbo.BindBase(GL_SHADER_STORAGE_BUFFER, 0);

    s_state->quad_shader.Bind();
    glUniformMatrix4fv(s_state->quad_proj_loc, 1, GL_FALSE, glm::value_ptr(s_state->projection));

    s_state->vao.Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);

    s_state->stats.quad_count += count;
    s_state->stats.draw_calls++;
}

static void FlushCircles()
{
    if (s_state->circle_buf.empty()) {
        return;
    }

    auto count = static_cast<GLsizei>(s_state->circle_buf.size());
    GLsizeiptr data_size = count * sizeof(CircleInstance);

    s_state->circle_ssbo.Bind(GL_SHADER_STORAGE_BUFFER);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data_size, nullptr, GL_STREAM_DRAW);
    void* ptr = glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(ptr, s_state->circle_buf.data(), data_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    s_state->circle_ssbo.BindBase(GL_SHADER_STORAGE_BUFFER, 1);

    s_state->circle_shader.Bind();
    glUniformMatrix4fv(s_state->circle_proj_loc, 1, GL_FALSE, glm::value_ptr(s_state->projection));

    s_state->vao.Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);

    s_state->stats.circle_count += count;
    s_state->stats.draw_calls++;
}

void Renderer2D::EndScene()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    FlushQuads();
    FlushCircles();

    glBindVertexArray(0);
    glUseProgram(0);
}

const Renderer2DStats& Renderer2D::GetStats()
{
    return s_state->stats;
}

void Renderer2D::ResetStats()
{
    s_state->stats = {};
}

} // namespace renderer
