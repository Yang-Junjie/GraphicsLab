#include "AdvanceLighting.hpp"

#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

AdvanceLighting::AdvanceLighting()
    : Scene("AdvanceLighting")
{}

void AdvanceLighting::OnEnter()
{
    shader_ = std::make_unique<gl::Shader>();
    shader_->CompileFromFile("shaders/AdvanceLighting/blinn_phong.vert",
                             "shaders/AdvanceLighting/blinn_phong.frag");

    lamp_shader_ = std::make_unique<gl::Shader>();
    lamp_shader_->CompileFromFile("shaders/BaseLighting/light.vert",
                                  "shaders/BaseLighting/light.frag");

    // Floor plane: large quad lying on the XZ plane at y=0
    // positions(3) + normals(3) + texcoords(2)
    float floor_vertices[] = {
        // positions            // normals         // texcoords
        10.0f, 0.0f, 10.0f, 0.0f,  1.0f,   0.0f, 10.0f,  0.0f, -10.0f, 0.0f, 10.0f,  0.0f,
        1.0f,  0.0f, 0.0f,  0.0f,  -10.0f, 0.0f, -10.0f, 0.0f, 1.0f,   0.0f, 0.0f,   10.0f,

        10.0f, 0.0f, 10.0f, 0.0f,  1.0f,   0.0f, 10.0f,  0.0f, -10.0f, 0.0f, -10.0f, 0.0f,
        1.0f,  0.0f, 0.0f,  10.0f, 10.0f,  0.0f, -10.0f, 0.0f, 1.0f,   0.0f, 10.0f,  10.0f,
    };

    floor_vao_ = std::make_unique<gl::VertexArray>();
    floor_vbo_ = std::make_unique<gl::Buffer>();

    floor_vao_->Bind();
    floor_vbo_->Upload(GL_ARRAY_BUFFER, sizeof(floor_vertices), floor_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    floor_vao_->Unbind();

    // Light cube geometry (reuse simple cube)
    float lamp_vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f,
        -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,
        0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f,
        0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f,
        0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
    };

    lamp_vao_ = std::make_unique<gl::VertexArray>();
    lamp_vbo_ = std::make_unique<gl::Buffer>();

    lamp_vao_->Bind();
    lamp_vbo_->Upload(GL_ARRAY_BUFFER, sizeof(lamp_vertices), lamp_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    lamp_vao_->Unbind();

    floor_texture_ = LoadTexture("res/image/get.png");

    glEnable(GL_DEPTH_TEST);

    camera_3d_ = std::make_unique<Camera3D>(camera_fov_, 4.0f / 3.0f, 0.1f, 100.0f);
    camera_3d_->SetPosition(camera_pos_);
    camera_3d_->SetRotation(camera_pitch_, camera_yaw_);
}

void AdvanceLighting::OnExit()
{
    glDisable(GL_DEPTH_TEST);

    if (floor_texture_) {
        glDeleteTextures(1, &floor_texture_);
        floor_texture_ = 0;
    }

    lamp_vbo_.reset();
    lamp_vao_.reset();
    floor_vbo_.reset();
    floor_vao_.reset();
    lamp_shader_.reset();
    shader_.reset();
    camera_3d_.reset();
    keys_down_.clear();
    right_mouse_down_ = false;
    first_mouse_ = true;
}

void AdvanceLighting::OnUpdate(float dt)
{
    if (!camera_3d_) {
        return;
    }

    float speed = camera_move_speed_;
    glm::vec3 front = camera_3d_->GetFront();
    glm::vec3 right = camera_3d_->GetRight();
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    if (keys_down_.count(GLFW_KEY_W) || keys_down_.count(GLFW_KEY_UP)) {
        camera_pos_ += front * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_S) || keys_down_.count(GLFW_KEY_DOWN)) {
        camera_pos_ -= front * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_D) || keys_down_.count(GLFW_KEY_RIGHT)) {
        camera_pos_ += right * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_A) || keys_down_.count(GLFW_KEY_LEFT)) {
        camera_pos_ -= right * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_SPACE)) {
        camera_pos_ += up * speed * dt;
    }
    if (keys_down_.count(GLFW_KEY_LEFT_SHIFT)) {
        camera_pos_ -= up * speed * dt;
    }
}

void AdvanceLighting::OnRender(float width, float height)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (!camera_3d_ || !shader_ || !lamp_shader_) {
        return;
    }

    camera_3d_->SetPosition(camera_pos_);
    camera_3d_->SetRotation(camera_pitch_, camera_yaw_);
    camera_3d_->SetPerspective(camera_fov_, width / height, 0.1f, 100.0f);

    glm::mat4 view = camera_3d_->GetViewMatrix();
    glm::mat4 projection = camera_3d_->GetProjectionMatrix();

    // Draw floor
    shader_->Bind();
    glUniformMatrix4fv(shader_->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shader_->Uniform("u_projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model(1.0f);
    glUniformMatrix4fv(shader_->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform3fv(shader_->Uniform("u_LightPos"), 1, glm::value_ptr(light_pos_));
    glUniform3fv(shader_->Uniform("u_ViewPos"), 1, glm::value_ptr(camera_pos_));
    glUniform3fv(shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light_color_));
    glUniform1f(shader_->Uniform("u_Shininess"), shininess_);
    glUniform1f(shader_->Uniform("u_AmbientStrength"), ambient_strength_);
    glUniform1f(shader_->Uniform("u_SpecularStrength"), specular_strength_);
    glUniform1i(shader_->Uniform("u_BlinnPhong"), use_blinn_phong_ ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floor_texture_);
    glUniform1i(shader_->Uniform("u_FloorTexture"), 0);

    floor_vao_->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    floor_vao_->Unbind();
    shader_->Unbind();

    // Draw light cube
    lamp_shader_->Bind();
    glUniformMatrix4fv(lamp_shader_->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
        lamp_shader_->Uniform("u_projection"), 1, GL_FALSE, glm::value_ptr(projection));

    model = glm::mat4(1.0f);
    model = glm::translate(model, light_pos_);
    model = glm::scale(model, glm::vec3(0.2f));
    glUniformMatrix4fv(lamp_shader_->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(lamp_shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light_color_));

    lamp_vao_->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    lamp_vao_->Unbind();
    lamp_shader_->Unbind();
}

void AdvanceLighting::OnRenderUI()
{
    ImGui::Text("Advanced Lighting: Blinn-Phong");
    ImGui::Separator();

    ImGui::Checkbox("Blinn-Phong (B key toggle)", &use_blinn_phong_);
    ImGui::Text("Current: %s", use_blinn_phong_ ? "Blinn-Phong" : "Phong");
    ImGui::Separator();

    ImGui::Text("Lighting");
    ImGui::SliderFloat("Shininess", &shininess_, 2.0f, 256.0f, "%.1f");
    ImGui::SliderFloat("Ambient Strength", &ambient_strength_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Specular Strength", &specular_strength_, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat3("Light Position", &light_pos_.x, 0.05f);
    ImGui::ColorEdit3("Light Color", &light_color_.x);

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::DragFloat3("Position", &camera_pos_.x, 0.05f);
    ImGui::SliderFloat("Pitch", &camera_pitch_, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_yaw_, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_fov_, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_move_speed_, 1.0f, 20.0f);

    ImGui::Separator();
    ImGui::TextWrapped("B: Toggle Blinn-Phong/Phong\n"
                       "WASD/Arrow: Move, Space/Shift: Up/Down\n"
                       "Scroll: FOV, RMB Drag: Look");
}

void AdvanceLighting::OnEvent(flux::Event& event)
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

GLuint AdvanceLighting::LoadTexture(const char* path)
{
    stbi_set_flip_vertically_on_load(true);
    int tex_w = 0;
    int tex_h = 0;
    int tex_channels = 0;
    unsigned char* data = stbi_load(path, &tex_w, &tex_h, &tex_channels, 0);
    if (!data) {
        return 0;
    }

    GLenum format = GL_RGB;
    if (tex_channels == 1) {
        format = GL_RED;
    } else if (tex_channels == 4) {
        format = GL_RGBA;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex_w, tex_h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return texture;
}

bool AdvanceLighting::OnKeyPressed(int keycode)
{
    if (keycode == GLFW_KEY_B) {
        use_blinn_phong_ = !use_blinn_phong_;
        return true;
    }
    keys_down_.insert(keycode);
    return false;
}

bool AdvanceLighting::OnKeyReleased(int keycode)
{
    keys_down_.erase(keycode);
    return false;
}

bool AdvanceLighting::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool AdvanceLighting::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool AdvanceLighting::OnMouseMoved(float x, float y)
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

    if (camera_3d_) {
        camera_yaw_ += dx * camera_mouse_sensitivity_;
        camera_pitch_ += dy * camera_mouse_sensitivity_;
        camera_pitch_ = glm::clamp(camera_pitch_, -89.0f, 89.0f);
    }

    return true;
}

bool AdvanceLighting::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    if (camera_3d_) {
        camera_fov_ -= y_offset * 2.0f;
        camera_fov_ = glm::clamp(camera_fov_, 10.0f, 120.0f);
        return true;
    }
    return false;
}
