#include "CirclePatternScene.hpp"
#include "Renderer/Renderer2D.hpp"

#include <cmath>

CirclePatternScene::CirclePatternScene()
    : Scene("Circle Pattern")
{}

void CirclePatternScene::OnEnter()
{
    time_ = 0.0f;
}

void CirclePatternScene::OnUpdate(float dt)
{
    time_ += dt;
}

void CirclePatternScene::OnRender(float width, float height)
{
    constexpr int ring_count = 3;
    constexpr int circles_per_ring = 8;

    float cx = width * 0.5f;
    float cy = height * 0.5f;
    float base_radius = (width < height ? width : height) * 0.08f;

    for (int ring = 0; ring < ring_count; ++ring) {
        float ring_dist = base_radius * 2.5f * static_cast<float>(ring + 1);
        float angle_offset = time_ * (1.0f + static_cast<float>(ring) * 0.3f);

        for (int i = 0; i < circles_per_ring; ++i) {
            float angle = angle_offset + static_cast<float>(i) /
                                             static_cast<float>(circles_per_ring) * 2.0f *
                                             3.14159265f;

            float x = cx + std::cos(angle) * ring_dist;
            float y = cy + std::sin(angle) * ring_dist;

            float hue = static_cast<float>(i) / static_cast<float>(circles_per_ring);
            float r = 0.5f + 0.5f * std::sin(hue * 6.28f + 0.0f);
            float g = 0.5f + 0.5f * std::sin(hue * 6.28f + 2.09f);
            float b = 0.5f + 0.5f * std::sin(hue * 6.28f + 4.19f);

            renderer::Renderer2D::DrawCircle({x, y}, base_radius, {r, g, b, 0.9f});
        }
    }

    // Center circle
    float pulse = 0.8f + 0.2f * std::sin(time_ * 3.0f);
    renderer::Renderer2D::DrawCircle(
        {cx, cy}, base_radius * 1.2f * pulse, {1.0f, 1.0f, 1.0f, 0.95f});
}
