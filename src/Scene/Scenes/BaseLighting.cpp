#include "BaseLighting.hpp"

#include <array>
#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>
#include <string>

namespace {

const std::array<glm::vec3, 10> kCubePositions = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(2.0f, 5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f, 3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f, 2.0f, -2.5f),
    glm::vec3(1.5f, 0.2f, -1.5f),
    glm::vec3(-1.3f, 1.0f, -1.5f),
};

const std::array<glm::vec3, 4> kDefaultPointLightPositions = {
    glm::vec3(0.7f, 0.2f, 2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f, 2.0f, -12.0f),
    glm::vec3(0.0f, 0.0f, -3.0f),
};

} // namespace

BaseLighting::BaseLighting()
    : Scene("Base Lighting")
{}

void BaseLighting::OnEnter()
{
    object_shader_ = std::make_unique<gl::Shader>();
    object_shader_->CompileFromFile("shaders/BaseLighting/camera.vert",
                                    "shaders/BaseLighting/camera.frag");

    lamp_shader_ = std::make_unique<gl::Shader>();
    lamp_shader_->CompileFromFile("shaders/BaseLighting/light.vert",
                                  "shaders/BaseLighting/light.frag");

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,
        0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  1.0f,
        0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,
        0.0f,  -1.0f, 0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

        -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
        0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,
        0.0f,  1.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, -1.0f,
        0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f,
        0.0f,  0.0f,  0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 1.0f,
        0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,
        0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 0.0f,
        -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,
        0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,
        -1.0f, 0.0f,  0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

        -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
        1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,
        1.0f,  0.0f,  0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

    cube_vao_ = std::make_unique<gl::VertexArray>();
    lamp_vao_ = std::make_unique<gl::VertexArray>();
    cube_vbo_ = std::make_unique<gl::Buffer>();

    cube_vbo_->Storage(sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Cube VAO: position + normal + texcoord
    glVertexArrayVertexBuffer(cube_vao_->Id(), 0, cube_vbo_->Id(), 0, 8 * sizeof(float));

    glEnableVertexArrayAttrib(cube_vao_->Id(), 0);
    glVertexArrayAttribFormat(cube_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(cube_vao_->Id(), 0, 0);

    glEnableVertexArrayAttrib(cube_vao_->Id(), 1);
    glVertexArrayAttribFormat(cube_vao_->Id(), 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(cube_vao_->Id(), 1, 0);

    glEnableVertexArrayAttrib(cube_vao_->Id(), 2);
    glVertexArrayAttribFormat(cube_vao_->Id(), 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glVertexArrayAttribBinding(cube_vao_->Id(), 2, 0);

    // Lamp VAO: same VBO, only position attribute
    glVertexArrayVertexBuffer(lamp_vao_->Id(), 0, cube_vbo_->Id(), 0, 8 * sizeof(float));

    glEnableVertexArrayAttrib(lamp_vao_->Id(), 0);
    glVertexArrayAttribFormat(lamp_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(lamp_vao_->Id(), 0, 0);

    diffuse_texture_ = LoadTexture("res/image/container2.png");
    specular_texture_ = LoadTexture("res/image/container2_specular.png");

    for (int i = 0; i < static_cast<int>(point_lights_.size()); ++i) {
        point_lights_[i].position = kDefaultPointLightPositions[i];
    }

    glEnable(GL_DEPTH_TEST);

    camera_3d_ = std::make_unique<Camera3D>(camera_3d_fov_, 4.0f / 3.0f, 0.1f, 100.0f);
    camera_3d_->SetPosition(camera_3d_pos_);
    camera_3d_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);
    active_camera_ = camera_3d_.get();
}

void BaseLighting::OnExit()
{
    glDisable(GL_DEPTH_TEST);

    if (diffuse_texture_) {
        glDeleteTextures(1, &diffuse_texture_);
        diffuse_texture_ = 0;
    }
    if (specular_texture_) {
        glDeleteTextures(1, &specular_texture_);
        specular_texture_ = 0;
    }

    cube_vbo_.reset();
    lamp_vao_.reset();
    cube_vao_.reset();
    lamp_shader_.reset();
    object_shader_.reset();

    camera_2d_.reset();
    camera_3d_.reset();
    active_camera_ = nullptr;
    keys_down_.clear();
    right_mouse_down_ = false;
    first_mouse_ = true;
}

void BaseLighting::OnUpdate(float dt)
{
    time_ += dt;

    if (camera_3d_) {
        float speed = camera_3d_move_speed_;
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

void BaseLighting::OnRender(float width, float height)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (!active_camera_ || !camera_3d_ || !object_shader_ || !lamp_shader_) {
        return;
    }

    camera_3d_->SetPosition(camera_3d_pos_);
    camera_3d_->SetRotation(camera_3d_pitch_, camera_3d_yaw_);
    camera_3d_->SetPerspective(camera_3d_fov_, width / height, 0.1f, 100.0f);

    glm::vec3 spot_position = spot_light_.follow_camera ? camera_3d_pos_ : spot_light_.position;
    glm::vec3 spot_direction =
        spot_light_.follow_camera ? camera_3d_->GetFront() : spot_light_.direction;
    if (glm::length(spot_direction) < 0.0001f) {
        spot_direction = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        spot_direction = glm::normalize(spot_direction);
    }

    object_shader_->Bind();

    glUniformMatrix4fv(object_shader_->Uniform("u_view"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(active_camera_->GetViewMatrix()));
    glUniformMatrix4fv(object_shader_->Uniform("u_projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(active_camera_->GetProjectionMatrix()));
    glUniform3fv(object_shader_->Uniform("u_ViewPos"), 1, glm::value_ptr(camera_3d_pos_));

    glUniform1i(object_shader_->Uniform("u_Material.diffuse"), 0);
    glUniform1i(object_shader_->Uniform("u_Material.specular"), 1);
    glUniform1f(object_shader_->Uniform("u_Material.shininess"), material_shininess_);

    glBindTextureUnit(0, diffuse_texture_);
    glBindTextureUnit(1, specular_texture_);

    glUniform1i(object_shader_->Uniform("u_DirLight.enabled"), dir_light_.enabled ? 1 : 0);
    glUniform3fv(
        object_shader_->Uniform("u_DirLight.direction"), 1, glm::value_ptr(dir_light_.direction));
    glUniform3fv(
        object_shader_->Uniform("u_DirLight.ambient"), 1, glm::value_ptr(dir_light_.ambient));
    glUniform3fv(
        object_shader_->Uniform("u_DirLight.diffuse"), 1, glm::value_ptr(dir_light_.diffuse));
    glUniform3fv(
        object_shader_->Uniform("u_DirLight.specular"), 1, glm::value_ptr(dir_light_.specular));

    for (int i = 0; i < static_cast<int>(point_lights_.size()); ++i) {
        const PointLight& light = point_lights_[i];
        std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
        glUniform1i(object_shader_->Uniform((prefix + "enabled").c_str()), light.enabled ? 1 : 0);
        glUniform3fv(object_shader_->Uniform((prefix + "position").c_str()),
                     1,
                     glm::value_ptr(light.position));
        glUniform3fv(object_shader_->Uniform((prefix + "ambient").c_str()),
                     1,
                     glm::value_ptr(light.ambient));
        glUniform3fv(object_shader_->Uniform((prefix + "diffuse").c_str()),
                     1,
                     glm::value_ptr(light.diffuse));
        glUniform3fv(object_shader_->Uniform((prefix + "specular").c_str()),
                     1,
                     glm::value_ptr(light.specular));
        glUniform1f(object_shader_->Uniform((prefix + "constant").c_str()), light.constant);
        glUniform1f(object_shader_->Uniform((prefix + "linear").c_str()), light.linear);
        glUniform1f(object_shader_->Uniform((prefix + "quadratic").c_str()), light.quadratic);
    }

    glUniform1i(object_shader_->Uniform("u_SpotLight.enabled"), spot_light_.enabled ? 1 : 0);
    glUniform3fv(object_shader_->Uniform("u_SpotLight.position"), 1, glm::value_ptr(spot_position));
    glUniform3fv(
        object_shader_->Uniform("u_SpotLight.direction"), 1, glm::value_ptr(spot_direction));
    glUniform3fv(
        object_shader_->Uniform("u_SpotLight.ambient"), 1, glm::value_ptr(spot_light_.ambient));
    glUniform3fv(
        object_shader_->Uniform("u_SpotLight.diffuse"), 1, glm::value_ptr(spot_light_.diffuse));
    glUniform3fv(
        object_shader_->Uniform("u_SpotLight.specular"), 1, glm::value_ptr(spot_light_.specular));
    glUniform1f(object_shader_->Uniform("u_SpotLight.constant"), spot_light_.constant);
    glUniform1f(object_shader_->Uniform("u_SpotLight.linear"), spot_light_.linear);
    glUniform1f(object_shader_->Uniform("u_SpotLight.quadratic"), spot_light_.quadratic);
    glUniform1f(object_shader_->Uniform("u_SpotLight.cutOff"),
                glm::cos(glm::radians(spot_light_.cut_off)));
    glUniform1f(object_shader_->Uniform("u_SpotLight.outerCutOff"),
                glm::cos(glm::radians(spot_light_.outer_cut_off)));

    cube_vao_->Bind();
    for (int i = 0; i < static_cast<int>(kCubePositions.size()); ++i) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, kCubePositions[i]);
        model = glm::rotate(
            model, glm::radians(20.0f * static_cast<float>(i)), glm::vec3(1.0f, 0.3f, 0.5f));
        glUniformMatrix4fv(object_shader_->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    cube_vao_->Unbind();

    object_shader_->Unbind();

    lamp_shader_->Bind();
    glUniformMatrix4fv(lamp_shader_->Uniform("u_view"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(active_camera_->GetViewMatrix()));
    glUniformMatrix4fv(lamp_shader_->Uniform("u_projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(active_camera_->GetProjectionMatrix()));

    lamp_vao_->Bind();
    for (int i = 0; i < static_cast<int>(point_lights_.size()); ++i) {
        const PointLight& light = point_lights_[i];
        if (!light.enabled) {
            continue;
        }

        glm::mat4 model(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(lamp_shader_->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(lamp_shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light.diffuse));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    lamp_vao_->Unbind();
    lamp_shader_->Unbind();
}

void BaseLighting::OnEvent(flux::Event& event)
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

GLuint BaseLighting::LoadTexture(const char* path)
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
    GLenum internal_format = GL_RGB8;
    if (tex_channels == 1) {
        format = GL_RED;
        internal_format = GL_R8;
    } else if (tex_channels == 4) {
        format = GL_RGBA;
        internal_format = GL_RGBA8;
    }

    int max_dim = (tex_w > tex_h) ? tex_w : tex_h;
    int levels = 1;
    while (max_dim > 1) { max_dim >>= 1; ++levels; }

    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(texture, levels, internal_format, tex_w, tex_h);
    glTextureSubImage2D(texture, 0, 0, 0, tex_w, tex_h, format, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(texture);

    stbi_image_free(data);
    return texture;
}

bool BaseLighting::OnKeyPressed(int keycode)
{
    keys_down_.insert(keycode);
    return false;
}

bool BaseLighting::OnKeyReleased(int keycode)
{
    keys_down_.erase(keycode);
    return false;
}

bool BaseLighting::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = true;
        first_mouse_ = true;
        return true;
    }
    return false;
}

bool BaseLighting::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        right_mouse_down_ = false;
        return true;
    }
    return false;
}

bool BaseLighting::OnMouseMoved(float x, float y)
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
        camera_3d_yaw_ += dx * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ += dy * camera_3d_mouse_sensitivity_;
        camera_3d_pitch_ = glm::clamp(camera_3d_pitch_, -89.0f, 89.0f);
    }

    return true;
}

bool BaseLighting::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    if (camera_3d_) {
        camera_3d_fov_ -= y_offset * 2.0f;
        camera_3d_fov_ = glm::clamp(camera_3d_fov_, 10.0f, 120.0f);
        return true;
    }

    return false;
}

void BaseLighting::OnRenderUI()
{
    ImGui::Text("LearnOpenGL Multiple Lights");
    ImGui::Separator();

    ImGui::Text("Camera");
    ImGui::DragFloat3("Position", &camera_3d_pos_.x, 0.05f);
    ImGui::SliderFloat("Pitch", &camera_3d_pitch_, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_3d_yaw_, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_3d_fov_, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_3d_move_speed_, 1.0f, 20.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera_3d_mouse_sensitivity_, 0.01f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Material");
    ImGui::SliderFloat("Shininess", &material_shininess_, 2.0f, 256.0f, "%.1f");

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled##Dir", &dir_light_.enabled);
        ImGui::DragFloat3("Direction", &dir_light_.direction.x, 0.01f);
        ImGui::ColorEdit3("Ambient##Dir", &dir_light_.ambient.x);
        ImGui::ColorEdit3("Diffuse##Dir", &dir_light_.diffuse.x);
        ImGui::ColorEdit3("Specular##Dir", &dir_light_.specular.x);
    }

    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < static_cast<int>(point_lights_.size()); ++i) {
            PointLight& light = point_lights_[i];
            std::string node_name = "Point Light " + std::to_string(i);
            if (ImGui::TreeNode(node_name.c_str())) {
                ImGui::PushID(i);
                ImGui::Checkbox("Enabled", &light.enabled);
                ImGui::DragFloat3("Position", &light.position.x, 0.05f);
                ImGui::ColorEdit3("Ambient", &light.ambient.x);
                ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
                ImGui::ColorEdit3("Specular", &light.specular.x);
                ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 2.0f, "%.3f");
                ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 1.0f, "%.3f");
                ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 1.0f, "%.3f");
                ImGui::PopID();
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled##Spot", &spot_light_.enabled);
        ImGui::Checkbox("Follow Camera", &spot_light_.follow_camera);
        if (!spot_light_.follow_camera) {
            ImGui::DragFloat3("Position##Spot", &spot_light_.position.x, 0.05f);
            ImGui::DragFloat3("Direction##Spot", &spot_light_.direction.x, 0.01f);
        }
        ImGui::ColorEdit3("Ambient##Spot", &spot_light_.ambient.x);
        ImGui::ColorEdit3("Diffuse##Spot", &spot_light_.diffuse.x);
        ImGui::ColorEdit3("Specular##Spot", &spot_light_.specular.x);
        ImGui::SliderFloat("Cut Off", &spot_light_.cut_off, 1.0f, 45.0f, "%.1f deg");
        ImGui::SliderFloat("Outer Cut Off", &spot_light_.outer_cut_off, 1.0f, 60.0f, "%.1f deg");
        if (spot_light_.outer_cut_off < spot_light_.cut_off) {
            spot_light_.outer_cut_off = spot_light_.cut_off;
        }
        ImGui::DragFloat("Constant##Spot", &spot_light_.constant, 0.01f, 0.0f, 2.0f, "%.3f");
        ImGui::DragFloat("Linear##Spot", &spot_light_.linear, 0.001f, 0.0f, 1.0f, "%.3f");
        ImGui::DragFloat("Quadratic##Spot", &spot_light_.quadratic, 0.001f, 0.0f, 1.0f, "%.3f");
    }

    ImGui::Separator();
    ImGui::TextWrapped("WASD/Arrow: Move, Space/Shift: Up/Down, Scroll: FOV, RMB Drag: Look");
}
