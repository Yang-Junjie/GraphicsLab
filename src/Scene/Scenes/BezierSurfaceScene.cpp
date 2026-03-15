#include "BezierSurfaceScene.hpp"

#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

BezierSurfaceScene::BezierSurfaceScene()
    : Scene("Bezier Surface")
{
    ResetControlPoints();
}

void BezierSurfaceScene::ResetControlPoints()
{
    // 4x4 grid on XZ plane in [-1.5, 1.5]^2 with hill-shaped Y values
    const float y_vals[16] = {
        0.0f,
        0.3f,
        0.3f,
        0.0f,
        0.3f,
        1.2f,
        1.2f,
        0.3f,
        0.3f,
        1.2f,
        1.2f,
        0.3f,
        0.0f,
        0.3f,
        0.3f,
        0.0f,
    };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float x = -1.5f + j * 1.0f;
            float z = -1.5f + i * 1.0f;
            control_points_[i * 4 + j] = glm::vec3(x, y_vals[i * 4 + j], z);
        }
    }
}

void BezierSurfaceScene::BuildCageIndices()
{
    // 4 rows × 3 horizontal segments + 4 columns × 3 vertical segments = 24 lines = 48 indices
    GLuint indices[48];
    int idx = 0;

    // Horizontal lines (along j for each row i)
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            indices[idx++] = i * 4 + j;
            indices[idx++] = i * 4 + j + 1;
        }
    }

    // Vertical lines (along i for each column j)
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            indices[idx++] = i * 4 + j;
            indices[idx++] = (i + 1) * 4 + j;
        }
    }

    glNamedBufferData(cage_ebo_, sizeof(indices), indices, GL_STATIC_DRAW);
}

void BezierSurfaceScene::UploadControlPoints()
{
    glNamedBufferSubData(surface_vbo_, 0, sizeof(glm::vec3) * 16, control_points_.data());
    glNamedBufferSubData(cage_vbo_, 0, sizeof(glm::vec3) * 16, control_points_.data());
}

void BezierSurfaceScene::OnEnter()
{
    surface_shader_.CompileFromFile("shaders/BezierSurfaceScene/surface.vert",
                                    "shaders/BezierSurfaceScene/surface.tcs",
                                    "shaders/BezierSurfaceScene/surface.tes",
                                    "shaders/BezierSurfaceScene/surface.frag");

    cage_shader_.CompileFromFile("shaders/BezierSurfaceScene/cage.vert",
                                 "shaders/BezierSurfaceScene/cage.frag");

    // --- Surface VAO/VBO (16 vec3 control points) ---
    glCreateBuffers(1, &surface_vbo_);
    glNamedBufferData(
        surface_vbo_, sizeof(glm::vec3) * 16, control_points_.data(), GL_DYNAMIC_DRAW);

    glCreateVertexArrays(1, &surface_vao_);
    glVertexArrayVertexBuffer(surface_vao_, 0, surface_vbo_, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(surface_vao_, 0);
    glVertexArrayAttribFormat(surface_vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(surface_vao_, 0, 0);

    // --- Cage VAO/VBO/EBO (same 16 points, drawn as GL_LINES) ---
    glCreateBuffers(1, &cage_vbo_);
    glNamedBufferData(cage_vbo_, sizeof(glm::vec3) * 16, control_points_.data(), GL_DYNAMIC_DRAW);

    glCreateBuffers(1, &cage_ebo_);

    glCreateVertexArrays(1, &cage_vao_);
    glVertexArrayVertexBuffer(cage_vao_, 0, cage_vbo_, 0, sizeof(glm::vec3));
    glVertexArrayElementBuffer(cage_vao_, cage_ebo_);
    glEnableVertexArrayAttrib(cage_vao_, 0);
    glVertexArrayAttribFormat(cage_vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(cage_vao_, 0, 0);

    BuildCageIndices();
}

void BezierSurfaceScene::OnExit()
{
    glDeleteBuffers(1, &surface_vbo_);
    glDeleteVertexArrays(1, &surface_vao_);
    glDeleteBuffers(1, &cage_vbo_);
    glDeleteBuffers(1, &cage_ebo_);
    glDeleteVertexArrays(1, &cage_vao_);
    surface_vbo_ = surface_vao_ = 0;
    cage_vbo_ = cage_ebo_ = cage_vao_ = 0;

    surface_shader_ = {};
    cage_shader_ = {};

    right_mouse_down_ = false;
    first_mouse_ = true;
}

void BezierSurfaceScene::OnRender(float width, float height)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    UploadControlPoints();

    // Orbit camera: compute position from spherical coordinates
    float pitch_rad = glm::radians(rot_x_);
    float yaw_rad = glm::radians(rot_y_);
    glm::vec3 cam_pos(cam_dist_ * cos(pitch_rad) * sin(yaw_rad),
                      cam_dist_ * sin(pitch_rad),
                      cam_dist_ * cos(pitch_rad) * cos(yaw_rad));

    float aspect = width / height;
    camera_.SetPerspective(45.0f, aspect, 0.1f, 100.0f);
    camera_.SetPosition(cam_pos);
    camera_.SetTarget(glm::vec3(0.0f));

    glm::mat4 view = camera_.GetViewMatrix();
    glm::mat4 projection = camera_.GetProjectionMatrix();
    glm::mat4 model(1.0f);
    glm::mat4 mvp = projection * view * model;

    // --- 1. Draw Bezier surface ---
    glPatchParameteri(GL_PATCH_VERTICES, 16);
    glBindVertexArray(surface_vao_);

    surface_shader_.Bind();
    glUniformMatrix4fv(surface_shader_.Uniform("u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(surface_shader_.Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(surface_shader_.Uniform("u_TessLevel"), tess_level_);
    glUniform3fv(surface_shader_.Uniform("u_ViewPos"), 1, glm::value_ptr(cam_pos));
    glUniform3fv(surface_shader_.Uniform("u_LightPos"), 1, glm::value_ptr(light_pos_));
    glUniform3f(surface_shader_.Uniform("u_LightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3fv(surface_shader_.Uniform("u_SurfaceColor"), 1, surface_color_);
    glUniform1f(surface_shader_.Uniform("u_Shininess"), shininess_);

    if (show_surface_) {
        glDrawArrays(GL_PATCHES, 0, 16);
    }

    // Optional wireframe overlay
    if (wireframe_) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform3f(surface_shader_.Uniform("u_SurfaceColor"), 1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_PATCHES, 0, 16);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    surface_shader_.Unbind();
    glBindVertexArray(0);

    // --- 2. Draw control cage ---
    if (show_cage_) {
        glBindVertexArray(cage_vao_);
        cage_shader_.Bind();
        glUniformMatrix4fv(cage_shader_.Uniform("u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(cage_shader_.Uniform("u_Color"), 0.5f, 0.5f, 0.5f);

        glDrawElements(GL_LINES, 48, GL_UNSIGNED_INT, nullptr);

        cage_shader_.Unbind();
        glBindVertexArray(0);
    }

    // --- 3. Draw control points ---
    if (show_points_) {
        glPointSize(point_size_);
        glBindVertexArray(cage_vao_);
        cage_shader_.Bind();
        glUniformMatrix4fv(cage_shader_.Uniform("u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(cage_shader_.Uniform("u_Color"), 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_POINTS, 0, 16);

        cage_shader_.Unbind();
        glBindVertexArray(0);
        glPointSize(1.0f);
    }

    glDisable(GL_DEPTH_TEST);
}

void BezierSurfaceScene::OnRenderUI()
{
    ImGui::Text("Bezier Surface (Tessellation)");
    ImGui::Separator();

    ImGui::SliderFloat("Tess Level", &tess_level_, 1.0f, 64.0f, "%.0f");
    ImGui::ColorEdit3("Surface Color", surface_color_);
    ImGui::SliderFloat("Shininess", &shininess_, 2.0f, 256.0f, "%.1f");
    ImGui::Checkbox("Wireframe", &wireframe_);
    ImGui::Checkbox("Show Cage", &show_cage_);
    ImGui::Checkbox("Show Points", &show_points_);
    ImGui::Checkbox("Show Surface", &show_surface_);
    if (show_points_) {
        ImGui::SliderFloat("Point Size", &point_size_, 2.0f, 20.0f);
    }

    ImGui::Separator();
    ImGui::DragFloat3("Light Pos", &light_pos_.x, 0.05f);
    ImGui::SliderFloat("Distance", &cam_dist_, 1.0f, 20.0f);

    ImGui::Separator();
    ImGui::Text("Control Point Y Values");

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int idx = i * 4 + j;
            char label[16];
            snprintf(label, sizeof(label), "P[%d][%d]", i, j);
            ImGui::SliderFloat(label, &control_points_[idx].y, -2.0f, 3.0f);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Control Points")) {
        ResetControlPoints();
    }

    ImGui::Separator();
    ImGui::TextWrapped("RMB Drag: Rotate\nScroll: Zoom");
}

void BezierSurfaceScene::OnEvent(flux::Event& event)
{
    flux::EventDispatcher dispatcher(event);

    dispatcher.Dispatch<flux::MouseButtonPressedEvent>([this](flux::MouseButtonPressedEvent& e) {
        return OnMouseButtonPressed(e.GetMouseButton());
    });
    dispatcher.Dispatch<flux::MouseButtonReleasedEvent>([this](flux::MouseButtonReleasedEvent& e) {
        return OnMouseButtonReleased(e.GetMouseButton());
    });
    dispatcher.Dispatch<flux::MouseMovedEvent>([this](flux::MouseMovedEvent& e) {
        return OnMouseMoved(e.GetX(), e.GetY());
    });
    dispatcher.Dispatch<flux::MouseScrolledEvent>([this](flux::MouseScrolledEvent& e) {
        return OnMouseScrolled(e.GetXOffset(), e.GetYOffset());
    });
}

bool BezierSurfaceScene::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool BezierSurfaceScene::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool BezierSurfaceScene::OnMouseMoved(float x, float y)
{
    if (!right_mouse_down_) {
        return false;
    }

    if (first_mouse_) {
        last_mouse_x_ = x;
        last_mouse_y_ = y;
        first_mouse_ = false;
        return true;
    }

    float dx = x - last_mouse_x_;
    float dy = y - last_mouse_y_;
    last_mouse_x_ = x;
    last_mouse_y_ = y;

    rot_y_ -= dx * mouse_sensitivity_;
    rot_x_ += dy * mouse_sensitivity_;
    rot_x_ = glm::clamp(rot_x_, -89.0f, 89.0f);

    return true;
}

bool BezierSurfaceScene::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    cam_dist_ -= y_offset * 0.5f;
    cam_dist_ = glm::clamp(cam_dist_, 1.0f, 20.0f);
    return true;
}
