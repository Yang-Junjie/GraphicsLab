#include "Backend/OpenGL/GL.hpp"
#include "Renderer2D.hpp"

#include <cstring>

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace renderer {

// ---- PersistentSSBO: triple-buffered persistent-mapped SSBO ----

static size_t RoundUp(size_t value, size_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

struct PersistentSSBO {
    GLuint id = 0;
    void* mapped = nullptr;
    uint32_t capacity = 0;
    size_t stride = 0;
    size_t partition_bytes = 0;
    GLsync fences[3] = {};
};

static void CreatePersistentSSBO(PersistentSSBO& buf, size_t stride, uint32_t capacity)
{
    GLint alignment = 0;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
    if (alignment < 1) {
        alignment = 256;
    }

    buf.stride = stride;
    buf.capacity = capacity;
    buf.partition_bytes = RoundUp(capacity * stride, static_cast<size_t>(alignment));

    GLsizeiptr total = static_cast<GLsizeiptr>(buf.partition_bytes) * 3;

    glGenBuffers(1, &buf.id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf.id);

    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, total, nullptr, flags);
    buf.mapped = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, total, flags);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

static void WaitFence(PersistentSSBO& buf, int index)
{
    if (buf.fences[index]) {
        while (true) {
            GLenum result =
                glClientWaitSync(buf.fences[index], GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000);
            if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
                break;
            }
        }
        glDeleteSync(buf.fences[index]);
        buf.fences[index] = nullptr;
    }
}

static void DestroyPersistentSSBO(PersistentSSBO& buf)
{
    if (!buf.id) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        WaitFence(buf, i);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf.id);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glDeleteBuffers(1, &buf.id);
    buf.id = 0;
    buf.mapped = nullptr;
}

static void GrowPersistentSSBO(PersistentSSBO& buf, uint32_t new_capacity)
{
    new_capacity = std::max(new_capacity, buf.capacity * 2);
    size_t stride = buf.stride;
    DestroyPersistentSSBO(buf);
    CreatePersistentSSBO(buf, stride, new_capacity);
}

static void PlaceFence(PersistentSSBO& buf, int index)
{
    if (buf.fences[index]) {
        glDeleteSync(buf.fences[index]);
    }
    buf.fences[index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

static void* PartitionPtr(PersistentSSBO& buf, int index)
{
    return static_cast<char*>(buf.mapped) + static_cast<size_t>(index) * buf.partition_bytes;
}

static GLintptr PartitionOffset(PersistentSSBO& buf, int index)
{
    return static_cast<GLintptr>(static_cast<size_t>(index) * buf.partition_bytes);
}

// ---- Internal State ----

struct RendererState {
    gl::VertexArray vao;
    gl::Buffer vbo;
    gl::Buffer ebo;

    gl::Shader quad_shader;
    gl::Shader circle_shader;

    PersistentSSBO quad_ssbo;
    PersistentSSBO circle_ssbo;

    GLint quad_proj_loc = -1;
    GLint circle_proj_loc = -1;

    std::vector<QuadInstance> quad_buf;
    std::vector<CircleInstance> circle_buf;

    uint32_t max_quads = 0;
    uint32_t max_circles = 0;

    int frame_index = 0;
    GLuint gpu_timer_queries[2] = {};
    int gpu_timer_index = 0;
    bool gpu_timer_started = false;

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

    // Persistent SSBOs with triple buffering
    CreatePersistentSSBO(s_state->quad_ssbo, sizeof(QuadInstance), max_quads);
    CreatePersistentSSBO(s_state->circle_ssbo, sizeof(CircleInstance), max_circles);

    // GPU timer queries (ping-pong)
    glGenQueries(2, s_state->gpu_timer_queries);

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
    DestroyPersistentSSBO(s_state->quad_ssbo);
    DestroyPersistentSSBO(s_state->circle_ssbo);
    glDeleteQueries(2, s_state->gpu_timer_queries);
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

QuadInstance* Renderer2D::AllocQuads(uint32_t count)
{
    auto& buf = s_state->quad_buf;
    size_t offset = buf.size();
    buf.resize(offset + count);
    return buf.data() + offset;
}

CircleInstance* Renderer2D::AllocCircles(uint32_t count)
{
    auto& buf = s_state->circle_buf;
    size_t offset = buf.size();
    buf.resize(offset + count);
    return buf.data() + offset;
}

static void FlushQuads()
{
    if (s_state->quad_buf.empty()) {
        return;
    }

    auto count = static_cast<GLsizei>(s_state->quad_buf.size());
    int fi = s_state->frame_index;

    if (static_cast<uint32_t>(count) > s_state->quad_ssbo.capacity) {
        GrowPersistentSSBO(s_state->quad_ssbo, static_cast<uint32_t>(count));
    }

    WaitFence(s_state->quad_ssbo, fi);

    std::memcpy(PartitionPtr(s_state->quad_ssbo, fi),
                s_state->quad_buf.data(),
                static_cast<size_t>(count) * sizeof(QuadInstance));

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER,
                      0,
                      s_state->quad_ssbo.id,
                      PartitionOffset(s_state->quad_ssbo, fi),
                      static_cast<GLsizeiptr>(count) * sizeof(QuadInstance));

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
    int fi = s_state->frame_index;

    if (static_cast<uint32_t>(count) > s_state->circle_ssbo.capacity) {
        GrowPersistentSSBO(s_state->circle_ssbo, static_cast<uint32_t>(count));
    }

    WaitFence(s_state->circle_ssbo, fi);

    std::memcpy(PartitionPtr(s_state->circle_ssbo, fi),
                s_state->circle_buf.data(),
                static_cast<size_t>(count) * sizeof(CircleInstance));

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER,
                      1,
                      s_state->circle_ssbo.id,
                      PartitionOffset(s_state->circle_ssbo, fi),
                      static_cast<GLsizeiptr>(count) * sizeof(CircleInstance));

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

    // GPU Timer: read previous frame result (non-blocking)
    if (s_state->gpu_timer_started) {
        int read_qi = 1 - s_state->gpu_timer_index;
        GLint available = 0;
        glGetQueryObjectiv(
            s_state->gpu_timer_queries[read_qi], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(s_state->gpu_timer_queries[read_qi], GL_QUERY_RESULT, &ns);
            s_state->stats.gpu_time_ms = static_cast<float>(ns) / 1'000'000.0f;
        }
    }

    // GPU Timer: begin this frame
    glBeginQuery(GL_TIME_ELAPSED, s_state->gpu_timer_queries[s_state->gpu_timer_index]);

    FlushQuads();
    FlushCircles();

    // GPU Timer: end this frame
    glEndQuery(GL_TIME_ELAPSED);
    s_state->gpu_timer_index = 1 - s_state->gpu_timer_index;
    s_state->gpu_timer_started = true;

    // Place fences for current partition
    int fi = s_state->frame_index;
    PlaceFence(s_state->quad_ssbo, fi);
    PlaceFence(s_state->circle_ssbo, fi);
    s_state->frame_index = (fi + 1) % 3;

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
