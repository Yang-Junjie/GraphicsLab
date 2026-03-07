#pragma once

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>
#include <string>

namespace renderer {

struct QuadInstance {
    glm::vec4 color;
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    float _pad[3];
};

static_assert(offsetof(QuadInstance, color) == 0);
static_assert(offsetof(QuadInstance, position) == 16);
static_assert(offsetof(QuadInstance, size) == 24);
static_assert(offsetof(QuadInstance, rotation) == 32);
static_assert(sizeof(QuadInstance) == 48);

struct CircleInstance {
    glm::vec4 color;
    glm::vec2 position;
    float radius;
    float _pad;
};

static_assert(offsetof(CircleInstance, color) == 0);
static_assert(offsetof(CircleInstance, position) == 16);
static_assert(offsetof(CircleInstance, radius) == 24);
static_assert(sizeof(CircleInstance) == 32);

struct Renderer2DStats {
    uint32_t quad_count = 0;
    uint32_t circle_count = 0;
    uint32_t draw_calls = 0;
};

class Renderer2D {
public:
    static void Init(const std::string& shader_dir,
                     uint32_t max_quads = 1'000'000,
                     uint32_t max_circles = 1'000'000);
    static void Shutdown();

    static void BeginScene(const glm::mat4& projection);
    static void EndScene();

    static void DrawQuad(const glm::vec2& position,
                         const glm::vec2& size,
                         float rotation,
                         const glm::vec4& color);
    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

    static void DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color);

    static const Renderer2DStats& GetStats();
    static void ResetStats();
};

} // namespace renderer
