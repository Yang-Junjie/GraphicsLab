#include "CameraTestScene.hpp"

#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

CameraTestScene::CameraTestScene()
    : Scene("Camera Test")
{}

void CameraTestScene::OnEnter()
{
    shader_ = std::make_unique<gl::Shader>();
    shader_->CompileFromFile("shaders/CameraTestScene/camera.vert",
                             "shaders/CameraTestScene/camera.frag");

    float vertices[] = {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
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
                        -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

    quad_vao_ = std::make_unique<gl::VertexArray>();
    quad_vbo_ = std::make_unique<gl::Buffer>();

    quad_vbo_->Storage(sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(quad_vao_->Id(), 0, quad_vbo_->Id(), 0, 5 * sizeof(float));

    glEnableVertexArrayAttrib(quad_vao_->Id(), 0);
    glVertexArrayAttribFormat(quad_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(quad_vao_->Id(), 0, 0);

    glEnableVertexArrayAttrib(quad_vao_->Id(), 1);
    glVertexArrayAttribFormat(quad_vao_->Id(), 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(quad_vao_->Id(), 1, 0);

    // Load texture
    stbi_set_flip_vertically_on_load(true);
    int tex_w, tex_h, tex_channels;
    unsigned char* data = stbi_load("res/image/container.jpg", &tex_w, &tex_h, &tex_channels, 0);
    if (data) {
        GLenum format = (tex_channels == 4) ? GL_RGBA : GL_RGB;
        GLenum internal_format = (tex_channels == 4) ? GL_RGBA8 : GL_RGB8;

        int max_dim = (tex_w > tex_h) ? tex_w : tex_h;
        int levels = 1;
        while (max_dim > 1) { max_dim >>= 1; ++levels; }

        glCreateTextures(GL_TEXTURE_2D, 1, &texture_);
        glTextureParameteri(texture_, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture_, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(texture_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture_, levels, internal_format, tex_w, tex_h);
        glTextureSubImage2D(texture_, 0, 0, 0, tex_w, tex_h, format, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(texture_);
        stbi_image_free(data);
    }

    glEnable(GL_DEPTH_TEST);
}

void CameraTestScene::OnExit()
{
    glDisable(GL_DEPTH_TEST);

    if (texture_) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }

    quad_vbo_.reset();
    quad_vao_.reset();
    shader_.reset();

    camera_2d_.reset();
    camera_3d_.reset();
    active_camera_ = nullptr;
    keys_down_.clear();
    right_mouse_down_ = false;
    first_mouse_ = true;
}

void CameraTestScene::OnUpdate(float dt)
{
    time_ += dt;

    // --- 2D camera keyboard movement ---
    if (camera_mode_ == CameraMode::Ortho2D && camera_2d_) {
        float speed = camera_2d_move_speed_ / camera_2d_zoom_;

        // Compute local axes based on camera rotation
        float rad = glm::radians(camera_2d_rotation_);
        glm::vec2 right_dir(cos(rad), sin(rad));
        glm::vec2 up_dir(-sin(rad), cos(rad));

        if (keys_down_.count(GLFW_KEY_W) || keys_down_.count(GLFW_KEY_UP)) {
            camera_2d_pos_ += up_dir * speed * dt;
        }
        if (keys_down_.count(GLFW_KEY_S) || keys_down_.count(GLFW_KEY_DOWN)) {
            camera_2d_pos_ -= up_dir * speed * dt;
        }
        if (keys_down_.count(GLFW_KEY_D) || keys_down_.count(GLFW_KEY_RIGHT)) {
            camera_2d_pos_ += right_dir * speed * dt;
        }
        if (keys_down_.count(GLFW_KEY_A) || keys_down_.count(GLFW_KEY_LEFT)) {
            camera_2d_pos_ -= right_dir * speed * dt;
        }

        if (keys_down_.count(GLFW_KEY_Q)) {
            camera_2d_rotation_ += camera_2d_rotate_speed_ * dt;
        }
        if (keys_down_.count(GLFW_KEY_E)) {
            camera_2d_rotation_ -= camera_2d_rotate_speed_ * dt;
        }
    }

    // --- 3D camera keyboard movement ---
    if ((camera_mode_ == CameraMode::Perspective3D || camera_mode_ == CameraMode::Orthographic3D) &&
        camera_3d_) {
        float speed = camera_3d_move_speed_;

        // Use camera's front/right vectors for movement
        glm::vec3 front = camera_3d_->GetFront();
        glm::vec3 right = camera_3d_->GetRight();
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
}

void CameraTestScene::OnRender(float width, float height)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (!active_camera_ || camera_mode_ == CameraMode::None) {
        return;
    }

    // Update camera parameters each frame
    if (camera_mode_ == CameraMode::Ortho2D && camera_2d_) {
        camera_2d_->SetAspectRatio(width / height);
        camera_2d_->SetPosition(camera_2d_pos_);
        camera_2d_->SetRotation(camera_2d_rotation_);
        camera_2d_->SetZoom(camera_2d_zoom_);
    } else if (camera_3d_) {
        camera_3d_->SetPosition(camera_3d_pos_);
        camera_3d_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);

        if (camera_mode_ == CameraMode::Perspective3D) {
            camera_3d_->SetPerspective(camera_3d_fov_, width / height, 0.1f, 100.0f);
        } else if (camera_mode_ == CameraMode::Orthographic3D) {
            float ortho_size = 5.0f;
            float aspect = width / height;
            camera_3d_->SetOrthographic(
                -ortho_size * aspect, ortho_size * aspect, -ortho_size, ortho_size, 0.1f, 100.0f);
        }
    }

    glm::mat4 view_proj = active_camera_->GetViewProjectionMatrix();

    shader_->Bind();

    GLint model_loc = shader_->Uniform("u_model");
    GLint view_loc = shader_->Uniform("u_view");
    GLint proj_loc = shader_->Uniform("u_projection");

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(active_camera_->GetViewMatrix()));
    glUniformMatrix4fv(
        proj_loc, 1, GL_FALSE, glm::value_ptr(active_camera_->GetProjectionMatrix()));

    glBindTextureUnit(0, texture_);
    glUniform1i(shader_->Uniform("u_Texture"), 0);

    quad_vao_->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    quad_vao_->Unbind();

    shader_->Unbind();
}

// ── Event dispatching ────────────────────────────────────────────────

void CameraTestScene::OnEvent(flux::Event& event)
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

bool CameraTestScene::OnKeyPressed(int keycode)
{
    keys_down_.insert(keycode);
    return false;
}

bool CameraTestScene::OnKeyReleased(int keycode)
{
    keys_down_.erase(keycode);
    return false;
}

bool CameraTestScene::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool CameraTestScene::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool CameraTestScene::OnMouseMoved(float x, float y)
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
    float dy = last_mouse_y_ - y; // y is inverted (screen coords)
    last_mouse_x_ = x;
    last_mouse_y_ = y;

    if (camera_3d_ &&
        (camera_mode_ == CameraMode::Perspective3D || camera_mode_ == CameraMode::Orthographic3D)) {
        // Right-drag to look around in 3D
        camera_3d_yaw_ += dx * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ += dy * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ = glm::clamp(camera_3d_pitch_, -89.0f, 89.0f);
    }

    return true;
}

bool CameraTestScene::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    if (camera_mode_ == CameraMode::Ortho2D && camera_2d_) {
        camera_2d_zoom_ *= (y_offset > 0) ? 1.1f : (1.0f / 1.1f);
        camera_2d_zoom_ = glm::clamp(camera_2d_zoom_, 0.01f, 100.0f);
        return true;
    }

    if ((camera_mode_ == CameraMode::Perspective3D || camera_mode_ == CameraMode::Orthographic3D) &&
        camera_3d_) {
        camera_3d_fov_ -= y_offset * 2.0f;
        camera_3d_fov_ = glm::clamp(camera_3d_fov_, 10.0f, 120.0f);
        return true;
    }

    return false;
}

// ── Camera setup ─────────────────────────────────────────────────────

void CameraTestScene::SetupCamera(CameraMode mode, float width, float height)
{
    camera_mode_ = mode;
    float aspect = width / height;

    switch (mode) {
        case CameraMode::Ortho2D:
            camera_2d_ = std::make_unique<Camera2D>(5.0f, aspect);
            active_camera_ = camera_2d_.get();
            break;

        case CameraMode::Perspective3D:
            camera_3d_ = std::make_unique<Camera3D>(camera_3d_fov_, aspect, 0.1f, 100.0f);
            camera_3d_->SetPosition(camera_3d_pos_);
            camera_3d_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);
            active_camera_ = camera_3d_.get();
            break;

        case CameraMode::Orthographic3D: {
            camera_3d_ = std::make_unique<Camera3D>();
            float ortho_size = 5.0f;
            camera_3d_->SetOrthographic(
                -ortho_size * aspect, ortho_size * aspect, -ortho_size, ortho_size, 0.1f, 100.0f);
            camera_3d_->SetPosition(camera_3d_pos_);
            camera_3d_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);
            active_camera_ = camera_3d_.get();
            break;
        }

        default:
            active_camera_ = nullptr;
            break;
    }
}

// ── UI ───────────────────────────────────────────────────────────────

void CameraTestScene::OnRenderUI()
{
    ImGui::Text("Camera Mode");
    ImGui::Separator();

    const char* modes[] = {"None", "2D Orthographic", "3D Perspective", "3D Orthographic"};
    int current_mode = static_cast<int>(camera_mode_);

    if (ImGui::Combo("Mode", &current_mode, modes, 4)) {
        CameraMode new_mode = static_cast<CameraMode>(current_mode);
        if (new_mode != camera_mode_) {
            SetupCamera(new_mode, 800.0f, 600.0f);
        }
    }

    ImGui::Separator();

    if (camera_mode_ == CameraMode::Ortho2D) {
        ImGui::Text("2D Camera Controls");
        ImGui::DragFloat2("Position", &camera_2d_pos_.x, 0.1f);
        ImGui::SliderFloat("Rotation", &camera_2d_rotation_, -180.0f, 180.0f);
        ImGui::SliderFloat("Zoom", &camera_2d_zoom_, 0.01f, 100.0f);
        ImGui::SliderFloat("Move Speed", &camera_2d_move_speed_, 1.0f, 20.0f);
        ImGui::SliderFloat("Rotate Speed", &camera_2d_rotate_speed_, 10.0f, 360.0f);

        ImGui::Separator();
        ImGui::TextWrapped("WASD/Arrows: Move | Q/E: Rotate | Scroll: Zoom");
    } else if (camera_mode_ == CameraMode::Perspective3D ||
               camera_mode_ == CameraMode::Orthographic3D) {
        ImGui::Text("3D Camera Controls");
        ImGui::DragFloat3("Position", &camera_3d_pos_.x, 0.1f);
        ImGui::SliderFloat("Pitch", &camera_3d_pitch_, -89.0f, 89.0f);
        ImGui::SliderFloat("Yaw", &camera_3d_yaw_, -180.0f, 360.0f);

        if (camera_mode_ == CameraMode::Perspective3D) {
            ImGui::SliderFloat("FOV", &camera_3d_fov_, 10.0f, 120.0f);
        }

        ImGui::SliderFloat("Move Speed", &camera_3d_move_speed_, 1.0f, 20.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &camera_3d_mouse_sensitivity_, 0.01f, 1.0f);

        ImGui::Separator();
        ImGui::TextWrapped(
            "WASD/Arrows: Move | Space/Shift: Up/Down | Scroll: FOV | RMB Drag: Look");
    }
}
