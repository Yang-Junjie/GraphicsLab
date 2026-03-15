#include "BezierCurveScene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

BezierCurveScene::BezierCurveScene()
    : Scene("Bezier Curve")
{
    // Default S-curve control points
    control_points_ = {{
        {-0.8f, -0.4f},
        {-0.3f, 0.8f},
        {0.3f, -0.8f},
        {0.8f, 0.4f},
    }};
}

void BezierCurveScene::OnEnter()
{
    bezier_shader_.CompileFromFile("shaders/BezierCurveScene/bezier.vert",
                                   "shaders/BezierCurveScene/bezier.tcs",
                                   "shaders/BezierCurveScene/bezier.tes",
                                   "shaders/BezierCurveScene/bezier.frag");

    overlay_shader_.CompileFromFile("shaders/BezierCurveScene/overlay.vert",
                                    "shaders/BezierCurveScene/overlay.frag");

    // Create VBO (4 control points, vec2 each = 32 bytes)
    glCreateBuffers(1, &vbo_);
    glNamedBufferData(vbo_, sizeof(glm::vec2) * 4, control_points_.data(), GL_DYNAMIC_DRAW);

    // Create VAO with DSA
    glCreateVertexArrays(1, &vao_);
    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(glm::vec2));

    glEnableVertexArrayAttrib(vao_, 0);
    glVertexArrayAttribFormat(vao_, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao_, 0, 0);
}

void BezierCurveScene::OnExit()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    vbo_ = vao_ = 0;

    bezier_shader_ = {};
    overlay_shader_ = {};
}

void BezierCurveScene::OnRender(float width, float height)
{
    float aspect = width / height;
    glm::mat4 proj = glm::ortho(-aspect, aspect, -1.0f, 1.0f);

    // Upload control points
    glNamedBufferSubData(vbo_, 0, sizeof(glm::vec2) * 4, control_points_.data());

    glBindVertexArray(vao_);

    // 1. Draw Bezier curve with tessellation
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glLineWidth(line_width_);

    bezier_shader_.Bind();
    glUniform1f(bezier_shader_.Uniform("u_TessLevel"), tess_level_);
    glUniformMatrix4fv(bezier_shader_.Uniform("u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(bezier_shader_.Uniform("u_Color"), 1, curve_color_);

    glDrawArrays(GL_PATCHES, 0, 4);
    bezier_shader_.Unbind();

    // 2. Draw control polygon
    if (show_polygon_) {
        glLineWidth(1.0f);
        overlay_shader_.Bind();
        glUniformMatrix4fv(
            overlay_shader_.Uniform("u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(overlay_shader_.Uniform("u_Color"), 0.5f, 0.5f, 0.5f);

        glDrawArrays(GL_LINE_STRIP, 0, 4);
        overlay_shader_.Unbind();
    }

    // 3. Draw control points
    if (show_points_) {
        glPointSize(point_size_);
        overlay_shader_.Bind();
        glUniformMatrix4fv(
            overlay_shader_.Uniform("u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(overlay_shader_.Uniform("u_Color"), 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_POINTS, 0, 4);
        overlay_shader_.Unbind();
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

void BezierCurveScene::OnRenderUI()
{
    ImGui::SliderFloat("Tessellation", &tess_level_, 1.0f, 64.0f, "%.0f");
    ImGui::ColorEdit3("Curve Color", curve_color_);
    ImGui::SliderFloat("Line Width", &line_width_, 1.0f, 10.0f);
    ImGui::Separator();

    for (int i = 0; i < 4; ++i) {
        char label[8];
        snprintf(label, sizeof(label), "P%d", i);
        ImGui::DragFloat2(label, &control_points_[i].x, 0.01f, -2.0f, 2.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Polygon", &show_polygon_);
    ImGui::Checkbox("Show Points", &show_points_);
    if (show_points_) {
        ImGui::SliderFloat("Point Size", &point_size_, 2.0f, 20.0f);
    }
}
