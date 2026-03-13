#include "AdvancedOpenGL.hpp"

#include <array>
#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

namespace {

struct DepthFuncOption {
    const char* name;
    GLenum value;
};

constexpr std::array<DepthFuncOption, 8> kDepthFuncOptions = {{
    {"GL_LESS (default)", GL_LESS},
    {"GL_LEQUAL", GL_LEQUAL},
    {"GL_GREATER", GL_GREATER},
    {"GL_GEQUAL", GL_GEQUAL},
    {"GL_EQUAL", GL_EQUAL},
    {"GL_NOTEQUAL", GL_NOTEQUAL},
    {"GL_ALWAYS", GL_ALWAYS},
    {"GL_NEVER", GL_NEVER},
}};

} // namespace

AdvancedOpenGL::AdvancedOpenGL()
    : Scene("Advanced OpenGL")
{}

void AdvancedOpenGL::OnEnter()
{
    shader_ = std::make_unique<gl::Shader>();
    if (!shader_->CompileFromFile("shaders/AdvancedOpenGL/depth_testing.vert",
                                  "shaders/AdvancedOpenGL/depth_testing.frag")) {
        status_text_ = "Shader compile failed";
        return;
    }

    outline_shader_ = std::make_unique<gl::Shader>();
    if (!outline_shader_->CompileFromFile("shaders/AdvancedOpenGL/outline.vert",
                                          "shaders/AdvancedOpenGL/outline.frag")) {
        status_text_ = "Outline shader compile failed";
        return;
    }

    float cube_vertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
        0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

        -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
        0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
        0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

        -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f,
    };

    float plane_vertices[] = {
        // positions          // texture Coords (note we set these higher than 1.0 for wrapping)
        5.0f, -0.5f, 5.0f,  2.0f,  0.0f,  -5.0f, -0.5f, 5.0f,
        0.0f, 0.0f,  -5.0f, -0.5f, -5.0f, 0.0f,  2.0f,

        5.0f, -0.5f, 5.0f,  2.0f,  0.0f,  -5.0f, -0.5f, -5.0f,
        0.0f, 2.0f,  5.0f,  -0.5f, -5.0f, 2.0f,  2.0f,
    };

    cube_vao_ = std::make_unique<gl::VertexArray>();
    cube_vbo_ = std::make_unique<gl::Buffer>();
    plane_vao_ = std::make_unique<gl::VertexArray>();
    plane_vbo_ = std::make_unique<gl::Buffer>();

    cube_vao_->Bind();
    cube_vbo_->Upload(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    cube_vao_->Unbind();

    plane_vao_->Bind();
    plane_vbo_->Upload(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    plane_vao_->Unbind();

    cube_texture_ = LoadTexture("res/image/marble.jpg");
    if (!cube_texture_) {
        cube_texture_ = LoadTexture("bin/res/image/marble.jpg");
    }

    floor_texture_ = LoadTexture("res/image/metal.png");
    if (!floor_texture_) {
        floor_texture_ = LoadTexture("bin/res/image/metal.png");
    }

    if (!cube_texture_ || !floor_texture_) {
        status_text_ = "Texture load failed (need marble.jpg + metal.png)";
    } else {
        status_text_ = "Loaded LearnOpenGL depth-testing scene";
    }

    camera_ = std::make_unique<Camera3D>(camera_3d_fov_, 16.0f / 9.0f, 0.1f, 100.0f);
    camera_->SetPosition(camera_3d_pos_);
    camera_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glDepthFunc(GL_LESS);
}

void AdvancedOpenGL::OnExit()
{
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    if (cube_texture_) {
        glDeleteTextures(1, &cube_texture_);
        cube_texture_ = 0;
    }
    if (floor_texture_) {
        glDeleteTextures(1, &floor_texture_);
        floor_texture_ = 0;
    }

    plane_vbo_.reset();
    cube_vbo_.reset();
    plane_vao_.reset();
    cube_vao_.reset();
    shader_.reset();
    outline_shader_.reset();
    camera_.reset();

    keys_down_.clear();
    right_mouse_down_ = false;
    first_mouse_ = true;
}

void AdvancedOpenGL::OnUpdate(float dt)
{
    if (!camera_) {
        return;
    }

    float speed = camera_3d_move_speed_;
    glm::vec3 front = camera_->GetFront();
    glm::vec3 right = camera_->GetRight();
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    if (keys_down_.count(GLFW_KEY_W) || keys_down_.count(GLFW_KEY_UP)) {
        camera_3d_pos_ += front * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_S) || keys_down_.count(GLFW_KEY_DOWN)) {
        camera_3d_pos_ -= front * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_D) || keys_down_.count(GLFW_KEY_RIGHT)) {
        camera_3d_pos_ += right * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_A) || keys_down_.count(GLFW_KEY_LEFT)) {
        camera_3d_pos_ -= right * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_SPACE)) {
        camera_3d_pos_ += up * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_LEFT_SHIFT)) {
        camera_3d_pos_ -= up * speed * dt;
    }
}

void AdvancedOpenGL::OnRender(float width, float height)
{
    if (!shader_ || !outline_shader_ || !camera_) {
        return;
    }

    const float safe_width = (width <= 0.0f) ? 1.0f : width;
    const float safe_height = (height <= 0.0f) ? 1.0f : height;

    camera_->SetPerspective(camera_3d_fov_, safe_width / safe_height, 0.1f, 100.0f);
    camera_->SetPosition(camera_3d_pos_);
    camera_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);

    if (depth_test_enabled_) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthFunc(kDepthFuncOptions[depth_func_index_].value);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    shader_->Bind();
    glUniformMatrix4fv(
        shader_->Uniform("u_View"), 1, GL_FALSE, glm::value_ptr(camera_->GetViewMatrix()));
    glUniformMatrix4fv(shader_->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(camera_->GetProjectionMatrix()));
    glUniform1i(shader_->Uniform("u_Texture"), 0);
    glActiveTexture(GL_TEXTURE0);

    // 1. Draw floor (no stencil write)
    glStencilMask(0x00);
    plane_vao_->Bind();
    glBindTexture(GL_TEXTURE_2D, floor_texture_);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(shader_->Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Draw cubes, write 1 to stencil buffer
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    cube_vao_->Bind();
    glBindTexture(GL_TEXTURE_2D, cube_texture_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
    glUniformMatrix4fv(shader_->Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(shader_->Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    shader_->Unbind();

    // 3. Draw scaled-up cubes with outline shader where stencil != 1
    if (outline_enabled_) {
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);

        outline_shader_->Bind();
        glUniformMatrix4fv(outline_shader_->Uniform("u_View"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(camera_->GetViewMatrix()));
        glUniformMatrix4fv(outline_shader_->Uniform("u_Projection"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(camera_->GetProjectionMatrix()));
        glUniform3fv(outline_shader_->Uniform("u_OutlineColor"), 1, outline_color_);

        cube_vao_->Bind();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(outline_scale_));
        glUniformMatrix4fv(
            outline_shader_->Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(outline_scale_));
        glUniformMatrix4fv(
            outline_shader_->Uniform("u_Model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        outline_shader_->Unbind();

        glEnable(GL_DEPTH_TEST);
    }

    // Reset stencil state
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AdvancedOpenGL::OnRenderUI()
{
    ImGui::TextWrapped("Load: %s", status_text_.c_str());
    ImGui::Separator();

    ImGui::Checkbox("Enable Depth Test", &depth_test_enabled_);
    if (ImGui::BeginCombo("Depth Func", kDepthFuncOptions[depth_func_index_].name)) {
        for (int i = 0; i < static_cast<int>(kDepthFuncOptions.size()); ++i) {
            bool selected = (i == depth_func_index_);
            if (ImGui::Selectable(kDepthFuncOptions[i].name, selected)) {
                depth_func_index_ = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();
    ImGui::Text("Stencil Outline");
    ImGui::Checkbox("Enable Outline", &outline_enabled_);
    if (outline_enabled_) {
        ImGui::SliderFloat("Outline Scale", &outline_scale_, 1.01f, 1.2f);
        ImGui::ColorEdit3("Outline Color", outline_color_);
    }

    ImGui::Separator();
    ImGui::Text("3D Camera");
    ImGui::DragFloat3("Position", &camera_3d_pos_.x, 0.01f);
    ImGui::SliderFloat("Pitch", &camera_3d_pitch_, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_3d_yaw_, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_3d_fov_, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_3d_move_speed_, 0.1f, 20.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera_3d_mouse_sensitivity_, 0.01f, 1.0f);
    ImGui::TextWrapped("WASD/Arrows: Move | Space/Shift: Up/Down | Scroll: FOV | RMB Drag: Look");
}

void AdvancedOpenGL::OnEvent(flux::Event& event)
{
    flux::EventDispatcher dispatcher(event);

    dispatcher.Dispatch<flux::KeyPressedEvent>([this](flux::KeyPressedEvent& e) {
        return OnKeyPressed(e.GetKeyCode());
    });
    dispatcher.Dispatch<flux::KeyReleasedEvent>([this](flux::KeyReleasedEvent& e) {
        return OnKeyReleased(e.GetKeyCode());
    });
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

GLuint AdvancedOpenGL::LoadTexture(const char* path, bool flip_vertical) const
{
    stbi_set_flip_vertically_on_load(flip_vertical ? 1 : 0);

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return texture;
}

bool AdvancedOpenGL::OnKeyPressed(int keycode)
{
    keys_down_.insert(keycode);
    return false;
}

bool AdvancedOpenGL::OnKeyReleased(int keycode)
{
    keys_down_.erase(keycode);
    return false;
}

bool AdvancedOpenGL::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool AdvancedOpenGL::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool AdvancedOpenGL::OnMouseMoved(float x, float y)
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
    float dy = last_mouse_y_ - y;
    last_mouse_x_ = x;
    last_mouse_y_ = y;

    if (camera_) {
        camera_3d_yaw_ += dx * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ += dy * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ = glm::clamp(camera_3d_pitch_, -89.0f, 89.0f);
    }

    return true;
}

bool AdvancedOpenGL::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    if (camera_) {
        camera_3d_fov_ -= y_offset * 2.0f;
        camera_3d_fov_ = glm::clamp(camera_3d_fov_, 10.0f, 120.0f);
        return true;
    }

    return false;
}
