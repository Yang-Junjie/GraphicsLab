#include "ModelScene.hpp"

#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace {
constexpr const char* kModelPath = "res/model/WaterBottle/WaterBottle.gltf";
}

ModelScene::ModelScene()
    : Scene("Model Viewer")
{}

void ModelScene::OnEnter()
{
    shader_ = std::make_unique<gl::Shader>();
    if (!shader_->CompileFromFile("shaders/ModelScene/model.vert",
                                  "shaders/ModelScene/model.frag")) {
        status_text_ = "Shader compile failed";
        model_loaded_ = false;
        return;
    }

    model_ = std::make_unique<Model>();
    model_loaded_ = model_->LoadFromGLTF(kModelPath);
    if (!model_loaded_) {
        status_text_ = "Model load failed: res/model/WaterBottle/WaterBottle.gltf";
    } else {
        status_text_ = "Model loaded";
    }

    camera_ = std::make_unique<Camera3D>(camera_3d_fov_, 16.0f / 9.0f, 0.01f, 10.0f);
    camera_->SetPosition(camera_3d_pos_);
    camera_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void ModelScene::OnExit()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    camera_.reset();
    model_.reset();
    shader_.reset();

    model_loaded_ = false;
    keys_down_.clear();
    right_mouse_down_ = false;
    first_mouse_ = true;
}

void ModelScene::OnUpdate(float dt)
{
    if (auto_rotate_) {
        rotation_y_deg_ += rotation_speed_deg_ * dt;
        if (rotation_y_deg_ > 360.0f) {
            rotation_y_deg_ -= 360.0f;
        }
    }

    if (camera_) {
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
}

void ModelScene::OnRender(float width, float height)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (!shader_ || !camera_ || !model_loaded_ || !model_) {
        return;
    }

    const float safe_height = (height <= 0.0f) ? 1.0f : height;

    camera_->SetPerspective(camera_3d_fov_, width / safe_height, 0.01f, 10.0f);
    camera_->SetPosition(camera_3d_pos_);
    camera_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);

    glm::vec3 light_dir = light_direction_;
    if (glm::length(light_dir) < 0.0001f) {
        light_dir = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        light_dir = glm::normalize(light_dir);
    }

    shader_->Bind();
    glUniformMatrix4fv(
        shader_->Uniform("u_View"), 1, GL_FALSE, glm::value_ptr(camera_->GetViewMatrix()));
    glUniformMatrix4fv(shader_->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(camera_->GetProjectionMatrix()));
    glUniform3fv(shader_->Uniform("u_CameraPos"), 1, glm::value_ptr(camera_->GetPosition()));
    glUniform3fv(shader_->Uniform("u_LightDirection"), 1, glm::value_ptr(light_dir));
    glUniform3fv(shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light_color_));
    glUniform3fv(shader_->Uniform("u_AmbientColor"), 1, glm::value_ptr(ambient_color_));

    glm::mat4 model_transform(1.0f);
    model_transform =
        glm::rotate(model_transform, glm::radians(rotation_y_deg_), glm::vec3(0.0f, 1.0f, 0.0f));
    model_transform = glm::scale(model_transform, glm::vec3(model_scale_));

    model_->Draw(*shader_, model_transform);

    shader_->Unbind();
}

void ModelScene::OnRenderUI()
{
    ImGui::TextWrapped("Load: %s", status_text_.c_str());
    if (model_ && model_loaded_) {
        ImGui::Text("Meshes: %llu | Materials: %llu | Instances: %llu",
                    static_cast<unsigned long long>(model_->GetMeshCount()),
                    static_cast<unsigned long long>(model_->GetMaterialCount()),
                    static_cast<unsigned long long>(model_->GetInstanceCount()));
    }

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &auto_rotate_);
    ImGui::SliderFloat("Rotation Y", &rotation_y_deg_, 0.0f, 360.0f, "%.1f deg");
    ImGui::SliderFloat("Rotate Speed", &rotation_speed_deg_, -180.0f, 180.0f, "%.1f deg/s");
    ImGui::SliderFloat("Model Scale", &model_scale_, 0.1f, 4.0f);

    ImGui::Separator();
    ImGui::Text("3D Camera");
    ImGui::DragFloat3("Position", &camera_3d_pos_.x, 0.01f);
    ImGui::SliderFloat("Pitch", &camera_3d_pitch_, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_3d_yaw_, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_3d_fov_, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_3d_move_speed_, 0.1f, 20.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera_3d_mouse_sensitivity_, 0.01f, 1.0f);
    ImGui::TextWrapped("WASD/Arrows: Move | Space/Shift: Up/Down | Scroll: FOV | RMB Drag: Look");

    ImGui::Separator();
    ImGui::DragFloat3("Light Dir", &light_direction_.x, 0.01f);
    ImGui::ColorEdit3("Light Color", &light_color_.x);
    ImGui::ColorEdit3("Ambient", &ambient_color_.x);
}

void ModelScene::OnEvent(flux::Event& event)
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

bool ModelScene::OnKeyPressed(int keycode)
{
    keys_down_.insert(keycode);
    return false;
}

bool ModelScene::OnKeyReleased(int keycode)
{
    keys_down_.erase(keycode);
    return false;
}

bool ModelScene::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool ModelScene::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool ModelScene::OnMouseMoved(float x, float y)
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

bool ModelScene::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    if (camera_) {
        camera_3d_fov_ -= y_offset * 2.0f;
        camera_3d_fov_ = glm::clamp(camera_3d_fov_, 10.0f, 120.0f);
        return true;
    }

    return false;
}
