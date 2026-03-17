#include "AdvanceLighting.hpp"
#include "Backend/OpenGL/Buffer.hpp"

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

    depth_shader_ = std::make_unique<gl::Shader>();
    depth_shader_->CompileFromFile("shaders/AdvanceLighting/shadow_mapping.vert",
                                   "shaders/AdvanceLighting/shadow_mapping.frag");

    // Floor plane: large quad lying on the XZ plane at y=0
    // positions(3) + normals(3) + texcoords(2)
    // clang-format off
    float floor_vertices[] = {
        // positions            // normals         // texcoords
         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
    };
    // clang-format on

    floor_vao_ = std::make_unique<gl::VertexArray>();
    floor_vbo_ = std::make_unique<gl::Buffer>();

    floor_vbo_->Storage(sizeof(floor_vertices), floor_vertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(floor_vao_->Id(), 0, floor_vbo_->Id(), 0, 8 * sizeof(float));

    glEnableVertexArrayAttrib(floor_vao_->Id(), 0);
    glVertexArrayAttribFormat(floor_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(floor_vao_->Id(), 0, 0);

    glEnableVertexArrayAttrib(floor_vao_->Id(), 1);
    glVertexArrayAttribFormat(floor_vao_->Id(), 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(floor_vao_->Id(), 1, 0);

    glEnableVertexArrayAttrib(floor_vao_->Id(), 2);
    glVertexArrayAttribFormat(floor_vao_->Id(), 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glVertexArrayAttribBinding(floor_vao_->Id(), 2, 0);

    // Scene cubes: positions(3) + normals(3) + texcoords(2)
    // clang-format off
    float scene_cube_vertices[] = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    };
    // clang-format on

    scene_cube_vao_ = std::make_unique<gl::VertexArray>();
    scene_cube_vbo_ = std::make_unique<gl::Buffer>();

    scene_cube_vbo_->Storage(sizeof(scene_cube_vertices), scene_cube_vertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(
        scene_cube_vao_->Id(), 0, scene_cube_vbo_->Id(), 0, 8 * sizeof(float));

    glEnableVertexArrayAttrib(scene_cube_vao_->Id(), 0);
    glVertexArrayAttribFormat(scene_cube_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(scene_cube_vao_->Id(), 0, 0);

    glEnableVertexArrayAttrib(scene_cube_vao_->Id(), 1);
    glVertexArrayAttribFormat(scene_cube_vao_->Id(), 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(scene_cube_vao_->Id(), 1, 0);

    glEnableVertexArrayAttrib(scene_cube_vao_->Id(), 2);
    glVertexArrayAttribFormat(scene_cube_vao_->Id(), 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glVertexArrayAttribBinding(scene_cube_vao_->Id(), 2, 0);

    // Light cube geometry (reuse simple cube, position only)
    // clang-format off
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
    // clang-format on

    cube_vao_ = std::make_unique<gl::VertexArray>();
    cube_vbo_ = std::make_unique<gl::Buffer>();

    cube_vbo_->Storage(sizeof(lamp_vertices), lamp_vertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(cube_vao_->Id(), 0, cube_vbo_->Id(), 0, 3 * sizeof(float));

    glEnableVertexArrayAttrib(cube_vao_->Id(), 0);
    glVertexArrayAttribFormat(cube_vao_->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(cube_vao_->Id(), 0, 0);

    floor_texture_ = LoadTexture("res/image/get.png");

    // Depth map texture for shadow mapping
    glCreateTextures(GL_TEXTURE_2D, 1, &depth_map_texture_);
    glTextureStorage2D(depth_map_texture_, 1, GL_DEPTH_COMPONENT24, shadow_width_, shadow_height_);
    glTextureParameteri(depth_map_texture_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(depth_map_texture_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(depth_map_texture_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(depth_map_texture_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(depth_map_texture_, GL_TEXTURE_BORDER_COLOR, border_color);

    // Depth map framebuffer
    glCreateFramebuffers(1, &depth_map_fbo_);
    glNamedFramebufferTexture(depth_map_fbo_, GL_DEPTH_ATTACHMENT, depth_map_texture_, 0);
    glNamedFramebufferDrawBuffer(depth_map_fbo_, GL_NONE);
    glNamedFramebufferReadBuffer(depth_map_fbo_, GL_NONE);

    glEnable(GL_DEPTH_TEST);

    camera_3d_ = std::make_unique<Camera3D>(camera_fov_, 4.0f / 3.0f, 0.1f, 100.0f);
    camera_3d_->SetPosition(camera_pos_);
    camera_3d_->SetRotation(camera_pitch_, camera_yaw_);
}

void AdvanceLighting::OnExit()
{
    glDisable(GL_DEPTH_TEST);

    if (depth_map_fbo_) {
        glDeleteFramebuffers(1, &depth_map_fbo_);
        depth_map_fbo_ = 0;
    }
    if (depth_map_texture_) {
        glDeleteTextures(1, &depth_map_texture_);
        depth_map_texture_ = 0;
    }
    if (floor_texture_) {
        glDeleteTextures(1, &floor_texture_);
        floor_texture_ = 0;
    }

    scene_cube_vbo_.reset();
    scene_cube_vao_.reset();
    cube_vbo_.reset();
    cube_vao_.reset();
    floor_vbo_.reset();
    floor_vao_.reset();
    depth_shader_.reset();
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

void AdvanceLighting::RenderScene(gl::Shader& shader)
{
    // Floor
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    floor_vao_->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    floor_vao_->Unbind();

    // Scene cubes
    scene_cube_vao_->Bind();

    // Cube 1: center
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Cube 2: right side, smaller
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.25f, 1.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Cube 3: left side, rotated
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.5f, 2.0f));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    scene_cube_vao_->Unbind();
}

void AdvanceLighting::OnRender(float width, float height)
{
    if (!camera_3d_ || !shader_ || !lamp_shader_ || !depth_shader_) {
        return;
    }

    camera_3d_->SetPosition(camera_pos_);
    camera_3d_->SetRotation(camera_pitch_, camera_yaw_);
    camera_3d_->SetPerspective(camera_fov_, width / height, 0.1f, 100.0f);

    // Compute light-space matrix (orthographic projection for directional light)
    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 light_projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 light_view = glm::lookAt(light_pos_, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 light_space_matrix = light_projection * light_view;

    // Save current FBO and viewport
    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_viewport[4];
    glGetIntegerv(GL_VIEWPORT, prev_viewport);

    // ====== Pass 1: Render depth map from light's perspective ======
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, shadow_width_, shadow_height_);
    glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
    glClear(GL_DEPTH_BUFFER_BIT);

    depth_shader_->Bind();
    glUniformMatrix4fv(depth_shader_->Uniform("u_LightSpaceMatrix"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(light_space_matrix));

    RenderScene(*depth_shader_);
    depth_shader_->Unbind();

    // ====== Pass 2: Render scene with shadow mapping ======
    // Restore app's FBO and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (selected_gamma_correction_ == 1) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    glm::mat4 view = camera_3d_->GetViewMatrix();
    glm::mat4 projection = camera_3d_->GetProjectionMatrix();

    shader_->Bind();
    glUniformMatrix4fv(shader_->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shader_->Uniform("u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(
        shader_->Uniform("u_LightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(light_space_matrix));

    glUniform3fv(shader_->Uniform("u_LightPos"), 1, glm::value_ptr(light_pos_));
    glUniform3fv(shader_->Uniform("u_ViewPos"), 1, glm::value_ptr(camera_pos_));
    glUniform3fv(shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light_color_));
    glUniform1f(shader_->Uniform("u_Shininess"), shininess_);
    glUniform1f(shader_->Uniform("u_AmbientStrength"), ambient_strength_);
    glUniform1f(shader_->Uniform("u_SpecularStrength"), specular_strength_);
    glUniform1i(shader_->Uniform("u_BlinnPhong"), use_blinn_phong_ ? 1 : 0);
    glUniform1i(shader_->Uniform("u_UseGammaCorrection"), selected_gamma_correction_ == 2 ? 1 : 0);
    glUniform1i(shader_->Uniform("u_EnableShadows"), enable_shadows_ ? 1 : 0);
    glUniform1i(shader_->Uniform("u_PCFregionSize"),
                pcf_region_size_ % 2 == 0
                    ? pcf_region_size_
                    : pcf_region_size_ + 1); // Ensure odd for symmetric sampling

    glBindTextureUnit(0, floor_texture_);
    glUniform1i(shader_->Uniform("u_FloorTexture"), 0);

    glBindTextureUnit(1, depth_map_texture_);
    glUniform1i(shader_->Uniform("u_ShadowMap"), 1);

    RenderScene(*shader_);
    shader_->Unbind();

    // ====== Pass 3: Render light cube visualization ======
    lamp_shader_->Bind();
    glUniformMatrix4fv(lamp_shader_->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
        lamp_shader_->Uniform("u_projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model(1.0f);
    model = glm::translate(model, light_pos_);
    model = glm::scale(model, glm::vec3(0.2f));
    glUniformMatrix4fv(lamp_shader_->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(lamp_shader_->Uniform("u_LightColor"), 1, glm::value_ptr(light_color_));

    cube_vao_->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    cube_vao_->Unbind();
    lamp_shader_->Unbind();
}

void AdvanceLighting::OnRenderUI()
{
    ImGui::Text("Advanced Lighting: Shadow Mapping");
    ImGui::Separator();

    ImGui::Checkbox("Blinn-Phong (B key toggle)", &use_blinn_phong_);
    ImGui::Text("Current: %s", use_blinn_phong_ ? "Blinn-Phong" : "Phong");

    if (ImGui::RadioButton("None", selected_gamma_correction_ == 0)) {
        selected_gamma_correction_ = 0;
    }
    if (ImGui::RadioButton("Gamma Correction with framebuffer sRGB",
                           selected_gamma_correction_ == 1)) {
        selected_gamma_correction_ = 1;
    }
    if (ImGui::RadioButton("Manual gamma correction in shader", selected_gamma_correction_ == 2)) {
        selected_gamma_correction_ = 2;
    }
    ImGui::Separator();

    ImGui::Text("Shadow Mapping");
    ImGui::Checkbox("Enable Shadows", &enable_shadows_);
    if (ImGui::SliderInt("PCF Region Size", &pcf_region_size_, 1, 11)) {
        if (pcf_region_size_ % 2 == 0) {
            pcf_region_size_ += 1;
        }
    }
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
    while (max_dim > 1) {
        max_dim >>= 1;
        ++levels;
    }

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
