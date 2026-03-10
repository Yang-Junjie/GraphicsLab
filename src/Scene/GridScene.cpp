#include "GridScene.hpp"
#include "Renderer/Renderer2D.hpp"

GridScene::GridScene()
    : Scene("Color Grid")
{}

void GridScene::OnRender(float width, float height)
{
    constexpr int cols = 12;
    constexpr int rows = 8;

    float cell_w = width / static_cast<float>(cols);
    float cell_h = height / static_cast<float>(rows);
    float size = (cell_w < cell_h ? cell_w : cell_h) * 0.85f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x = (static_cast<float>(c) + 0.5f) * cell_w;
            float y = (static_cast<float>(r) + 0.5f) * cell_h;

            float red = static_cast<float>(c) / static_cast<float>(cols - 1);
            float green = static_cast<float>(r) / static_cast<float>(rows - 1);
            float blue = 1.0f - red * 0.5f;

            renderer::Renderer2D::DrawQuad({x, y}, {size, size}, {red, green, blue, 1.0f});
        }
    }
}
