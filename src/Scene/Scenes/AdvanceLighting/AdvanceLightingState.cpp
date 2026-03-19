#include "Scene/Scenes/AdvanceLighting/AdvanceLightingState.hpp"

#include <algorithm>
#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>
#include <string>

namespace {

struct FramebufferState {
    GLint framebuffer = 0;
    std::array<GLint, 4> viewport = {};
};

FramebufferState CaptureFramebufferState()
{
    FramebufferState state;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &state.framebuffer);
    glGetIntegerv(GL_VIEWPORT, state.viewport.data());
    return state;
}

glm::vec3 HdrLightColor(const AdvanceLightingPointLight& light)
{
    return light.color * light.intensity;
}

const AdvanceLightingPointLight& PrimaryLight(const AdvanceLightingLightingSettings& lighting)
{
    return lighting.lights[0];
}

} // namespace

AdvanceLightingLightingSettings::AdvanceLightingLightingSettings()
{
    lights = {{
        {glm::vec3(-1.5f, 3.8f, 2.0f), glm::vec3(1.0f, 0.96f, 0.90f), 3.0f},
        {glm::vec3(2.5f, 1.4f, 1.0f), glm::vec3(0.25f, 0.55f, 1.0f), 3.0f},
        {glm::vec3(-2.8f, 1.2f, -1.8f), glm::vec3(1.0f, 0.35f, 0.25f), 3.0f},
        {glm::vec3(0.0f, 2.7f, -3.0f), glm::vec3(0.35f, 1.0f, 0.45f), 3.0f},
        {glm::vec3(4.0f, 0.8f, -4.0f), glm::vec3(1.0f, 0.75f, 0.20f), 3.0f},
        {glm::vec3(-4.0f, 0.8f, 4.0f), glm::vec3(1.0f, 0.20f, 0.85f), 3.0f},
        {glm::vec3(3.0f, 2.8f, -1.0f), glm::vec3(0.40f, 1.0f, 0.95f), 3.0f},
        {glm::vec3(-1.0f, 1.8f, 3.5f), glm::vec3(1.0f, 0.55f, 0.15f), 3.0f},
    }};
}

void AdvanceLightingState::OnEnter()
{
    CreateShaders();
    CreateGeometry();

    resources_.floor_texture = LoadTexture("res/image/get.png");
    CreateShadowMap();

    glEnable(GL_DEPTH_TEST);

    camera_.camera.SetPosition(camera_.position);
    camera_.camera.SetRotation(camera_.pitch, camera_.yaw);
}

void AdvanceLightingState::OnExit()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);

    DestroyBloomFBOs();
    DestroyGBuffer();
    DestroyShadowMap();

    if (resources_.floor_texture) {
        glDeleteTextures(1, &resources_.floor_texture);
        resources_.floor_texture = 0;
    }

    resources_.quad_vbo.reset();
    resources_.quad_vao.reset();
    resources_.scene_cube_vbo.reset();
    resources_.scene_cube_vao.reset();
    resources_.cube_vbo.reset();
    resources_.cube_vao.reset();
    resources_.floor_vbo.reset();
    resources_.floor_vao.reset();

    resources_.bloom_composite_shader.reset();
    resources_.bloom_blur_shader.reset();
    resources_.depth_shader.reset();
    resources_.lamp_shader.reset();
    resources_.lighting_shader.reset();
    resources_.geometry_shader.reset();

    input_.keys_down.clear();
    input_.right_mouse_down = false;
    input_.first_mouse = true;
}

void AdvanceLightingState::OnUpdate(float dt)
{
    const float speed = camera_.move_speed;
    const glm::vec3 front = camera_.camera.GetFront();
    const glm::vec3 right = camera_.camera.GetRight();
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    if (input_.keys_down.count(GLFW_KEY_W) || input_.keys_down.count(GLFW_KEY_UP)) {
        camera_.position += front * speed * dt;
    }
    if (input_.keys_down.count(GLFW_KEY_S) || input_.keys_down.count(GLFW_KEY_DOWN)) {
        camera_.position -= front * speed * dt;
    }
    if (input_.keys_down.count(GLFW_KEY_D) || input_.keys_down.count(GLFW_KEY_RIGHT)) {
        camera_.position += right * speed * dt;
    }
    if (input_.keys_down.count(GLFW_KEY_A) || input_.keys_down.count(GLFW_KEY_LEFT)) {
        camera_.position -= right * speed * dt;
    }
    if (input_.keys_down.count(GLFW_KEY_SPACE)) {
        camera_.position += up * speed * dt;
    }
    if (input_.keys_down.count(GLFW_KEY_LEFT_SHIFT)) {
        camera_.position -= up * speed * dt;
    }
}

void AdvanceLightingState::OnRender(float width, float height)
{
    if (!resources_.geometry_shader || !resources_.lighting_shader || !resources_.lamp_shader ||
        !resources_.depth_shader || !resources_.floor_vao || !resources_.scene_cube_vao ||
        !resources_.cube_vao) {
        return;
    }

    SyncCamera(width, height);

    const bool use_hdr_buffer = post_process_.enable_hdr || post_process_.enable_bloom;
    const int viewport_width = std::max(static_cast<int>(width), 0);
    const int viewport_height = std::max(static_cast<int>(height), 0);
    if (viewport_width <= 0 || viewport_height <= 0) {
        return;
    }

    EnsureGBuffer(viewport_width, viewport_height);
    EnsureBloomBuffers(viewport_width, viewport_height, use_hdr_buffer);

    const FramebufferState framebuffers = CaptureFramebufferState();
    const glm::mat4 light_space_matrix = BuildLightSpaceMatrix();
    const glm::mat4 view = camera_.camera.GetViewMatrix();
    const glm::mat4 projection = camera_.camera.GetProjectionMatrix();

    RenderShadowPass(light_space_matrix);
    RenderGeometryPass(viewport_width, viewport_height, view, projection);
    RenderLightingPass(
        use_hdr_buffer, framebuffers.viewport, framebuffers.framebuffer, light_space_matrix);
    BlitGBufferDepth(use_hdr_buffer, framebuffers.viewport, framebuffers.framebuffer);
    RenderLightCube(use_hdr_buffer, view, projection);

    if (use_hdr_buffer) {
        RenderPostProcess(framebuffers.viewport, framebuffers.framebuffer);
    }
}

void AdvanceLightingState::OnRenderUI()
{
    lighting_.active_light_count =
        std::clamp(lighting_.active_light_count, 1, AdvanceLightingLightingSettings::kMaxLights);

    auto render_preview = [this](const char* hint) {
        GLuint preview_texture = 0;
        switch (debug_.selected_gbuffer_preview) {
            case 0:
                preview_texture = resources_.g_position_texture;
                break;
            case 1:
                preview_texture = resources_.g_normal_texture;
                break;
            case 2:
                preview_texture = resources_.g_albedo_spec_texture;
                break;
            default:
                break;
        }

        if (!preview_texture || resources_.g_buffer_width <= 0 || resources_.g_buffer_height <= 0) {
            ImGui::TextDisabled("G-buffer unavailable before first render.");
            return;
        }

        const float available_width = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
        const float aspect = static_cast<float>(resources_.g_buffer_height) /
                             static_cast<float>(resources_.g_buffer_width);
        const float preview_height = std::min(available_width * aspect, 220.0f);
        const float preview_width = preview_height / std::max(aspect, 0.001f);

        ImGui::Image(static_cast<ImTextureID>(preview_texture),
                     ImVec2(preview_width, preview_height),
                     ImVec2(0, 1),
                     ImVec2(1, 0));
        ImGui::TextWrapped("%s", hint);
    };

    ImGui::Text("Advanced Lighting: Deferred Rendering + Shadow Mapping");
    ImGui::Separator();

    ImGui::Checkbox("Blinn-Phong (B key toggle)", &lighting_.use_blinn_phong);
    ImGui::Text("Current: %s", lighting_.use_blinn_phong ? "Blinn-Phong" : "Phong");

    if (ImGui::RadioButton("None", post_process_.selected_gamma_correction == 0)) {
        post_process_.selected_gamma_correction = 0;
    }
    if (ImGui::RadioButton("Gamma Correction with framebuffer sRGB",
                           post_process_.selected_gamma_correction == 1)) {
        post_process_.selected_gamma_correction = 1;
    }
    if (ImGui::RadioButton("Manual gamma correction in shader",
                           post_process_.selected_gamma_correction == 2)) {
        post_process_.selected_gamma_correction = 2;
    }
    ImGui::Separator();

    ImGui::Text("Shadow Mapping");
    ImGui::Checkbox("Enable Shadows", &shadow_.enable_shadows);
    if (ImGui::SliderInt("PCF Region Size", &shadow_.pcf_region_size, 1, 11)) {
        shadow_.pcf_region_size = NormalizedPcfRegionSize();
    }
    ImGui::Separator();

    ImGui::Text("HDR Tone Mapping");
    ImGui::Checkbox("Enable HDR", &post_process_.enable_hdr);
    if (post_process_.enable_hdr) {
        const char* modes[] = {"Reinhard", "Exposure"};
        ImGui::Combo("Tone Mapping", &post_process_.tone_mapping_mode, modes, 2);
        if (post_process_.tone_mapping_mode == 1) {
            ImGui::SliderFloat("Exposure", &post_process_.exposure, 0.1f, 10.0f, "%.2f");
        }
    }
    ImGui::Separator();

    ImGui::Text("Bloom");
    ImGui::Checkbox("Enable Bloom", &post_process_.enable_bloom);
    if (post_process_.enable_bloom) {
        ImGui::SliderFloat("Bloom Threshold", &post_process_.bloom_threshold, 0.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("Bloom Intensity", &post_process_.bloom_intensity, 0.0f, 5.0f, "%.2f");
        ImGui::SliderInt("Blur Iterations", &post_process_.bloom_blur_iterations, 1, 20);
    }
    ImGui::Separator();

    ImGui::Text("Lighting");
    ImGui::SliderFloat("Shininess", &lighting_.shininess, 2.0f, 256.0f, "%.1f");
    ImGui::SliderFloat("Ambient Strength", &lighting_.ambient_strength, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Specular Strength", &lighting_.specular_strength, 0.0f, 2.0f, "%.2f");
    ImGui::SliderInt("Active Lights",
                     &lighting_.active_light_count,
                     1,
                     AdvanceLightingLightingSettings::kMaxLights);
    ImGui::TextDisabled("Light 0 is the shadow-casting light.");
    for (int i = 0; i < lighting_.active_light_count; ++i) {
        auto& light = lighting_.lights[i];
        std::string label = (i == 0) ? "Light 0 (Shadow)" : ("Light " + std::to_string(i));
        if (ImGui::TreeNode(label.c_str())) {
            std::string position_label = "Position##light_" + std::to_string(i);
            std::string color_label = "Color##light_" + std::to_string(i);
            std::string intensity_label = "Intensity##light_" + std::to_string(i);
            ImGui::DragFloat3(position_label.c_str(), &light.position.x, 0.05f);
            ImGui::ColorEdit3(color_label.c_str(), &light.color.x);
            ImGui::SliderFloat(intensity_label.c_str(), &light.intensity, 0.0f, 20.0f, "%.1f");
            ImGui::TreePop();
        }
    }

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::DragFloat3("Position", &camera_.position.x, 0.05f);
    ImGui::SliderFloat("Pitch", &camera_.pitch, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_.yaw, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_.fov, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_.move_speed, 1.0f, 20.0f);

    ImGui::Separator();
    ImGui::Text("G-Buffer Preview");
    const char* gbuffer_previews[] = {"Position", "Normal", "Albedo + Spec"};
    ImGui::Combo("Attachment", &debug_.selected_gbuffer_preview, gbuffer_previews, 3);
    switch (debug_.selected_gbuffer_preview) {
        case 0:
            render_preview(
                "Position buffer contains world-space coordinates, so this preview is unclamped.");
            break;
        case 1:
            render_preview(
                "Normal buffer is encoded to 0..1 for preview and decoded back in lighting pass.");
            break;
        case 2:
            render_preview(
                "RGB stores albedo and A stores the specular mask / geometry occupancy.");
            break;
        default:
            break;
    }

    ImGui::Separator();
    ImGui::TextWrapped("B: Toggle Blinn-Phong/Phong\n"
                       "Deferred: Geometry Pass + Lighting Pass\n"
                       "WASD/Arrow: Move, Space/Shift: Up/Down\n"
                       "Scroll: FOV, RMB Drag: Look");
}

void AdvanceLightingState::OnEvent(flux::Event& event)
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

void AdvanceLightingState::CreateShaders()
{
    resources_.geometry_shader = std::make_unique<gl::Shader>();
    resources_.geometry_shader->CompileFromFile("shaders/AdvanceLighting/deferred_geometry.vert",
                                                "shaders/AdvanceLighting/deferred_geometry.frag");

    resources_.lighting_shader = std::make_unique<gl::Shader>();
    resources_.lighting_shader->CompileFromFile("shaders/AdvanceLighting/bloom_quad.vert",
                                                "shaders/AdvanceLighting/deferred_lighting.frag");

    resources_.lamp_shader = std::make_unique<gl::Shader>();
    resources_.lamp_shader->CompileFromFile("shaders/BaseLighting/light.vert",
                                            "shaders/AdvanceLighting/bloom_light_box.frag");

    resources_.depth_shader = std::make_unique<gl::Shader>();
    resources_.depth_shader->CompileFromFile("shaders/AdvanceLighting/shadow_mapping.vert",
                                             "shaders/AdvanceLighting/shadow_mapping.frag");

    resources_.bloom_blur_shader = std::make_unique<gl::Shader>();
    resources_.bloom_blur_shader->CompileFromFile("shaders/AdvanceLighting/bloom_quad.vert",
                                                  "shaders/AdvanceLighting/bloom_blur.frag");

    resources_.bloom_composite_shader = std::make_unique<gl::Shader>();
    resources_.bloom_composite_shader->CompileFromFile(
        "shaders/AdvanceLighting/bloom_quad.vert", "shaders/AdvanceLighting/bloom_composite.frag");
}

void AdvanceLightingState::CreateGeometry()
{
    // clang-format off
    float floor_vertices[] = {
         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
    };

    float scene_cube_vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    };

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

    float quad_vertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };
    // clang-format on

    resources_.floor_vao = std::make_unique<gl::VertexArray>();
    resources_.floor_vbo = std::make_unique<gl::Buffer>();
    resources_.floor_vbo->Storage(sizeof(floor_vertices), floor_vertices, GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(
        resources_.floor_vao->Id(), 0, resources_.floor_vbo->Id(), 0, 8 * sizeof(float));
    glEnableVertexArrayAttrib(resources_.floor_vao->Id(), 0);
    glVertexArrayAttribFormat(resources_.floor_vao->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(resources_.floor_vao->Id(), 0, 0);
    glEnableVertexArrayAttrib(resources_.floor_vao->Id(), 1);
    glVertexArrayAttribFormat(
        resources_.floor_vao->Id(), 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(resources_.floor_vao->Id(), 1, 0);
    glEnableVertexArrayAttrib(resources_.floor_vao->Id(), 2);
    glVertexArrayAttribFormat(
        resources_.floor_vao->Id(), 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glVertexArrayAttribBinding(resources_.floor_vao->Id(), 2, 0);

    resources_.scene_cube_vao = std::make_unique<gl::VertexArray>();
    resources_.scene_cube_vbo = std::make_unique<gl::Buffer>();
    resources_.scene_cube_vbo->Storage(
        sizeof(scene_cube_vertices), scene_cube_vertices, GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(
        resources_.scene_cube_vao->Id(), 0, resources_.scene_cube_vbo->Id(), 0, 8 * sizeof(float));
    glEnableVertexArrayAttrib(resources_.scene_cube_vao->Id(), 0);
    glVertexArrayAttribFormat(resources_.scene_cube_vao->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(resources_.scene_cube_vao->Id(), 0, 0);
    glEnableVertexArrayAttrib(resources_.scene_cube_vao->Id(), 1);
    glVertexArrayAttribFormat(
        resources_.scene_cube_vao->Id(), 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(resources_.scene_cube_vao->Id(), 1, 0);
    glEnableVertexArrayAttrib(resources_.scene_cube_vao->Id(), 2);
    glVertexArrayAttribFormat(
        resources_.scene_cube_vao->Id(), 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glVertexArrayAttribBinding(resources_.scene_cube_vao->Id(), 2, 0);

    resources_.cube_vao = std::make_unique<gl::VertexArray>();
    resources_.cube_vbo = std::make_unique<gl::Buffer>();
    resources_.cube_vbo->Storage(sizeof(lamp_vertices), lamp_vertices, GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(
        resources_.cube_vao->Id(), 0, resources_.cube_vbo->Id(), 0, 3 * sizeof(float));
    glEnableVertexArrayAttrib(resources_.cube_vao->Id(), 0);
    glVertexArrayAttribFormat(resources_.cube_vao->Id(), 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(resources_.cube_vao->Id(), 0, 0);

    resources_.quad_vao = std::make_unique<gl::VertexArray>();
    resources_.quad_vbo = std::make_unique<gl::Buffer>();
    resources_.quad_vbo->Storage(sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(
        resources_.quad_vao->Id(), 0, resources_.quad_vbo->Id(), 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(resources_.quad_vao->Id(), 0);
    glVertexArrayAttribFormat(resources_.quad_vao->Id(), 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(resources_.quad_vao->Id(), 0, 0);
    glEnableVertexArrayAttrib(resources_.quad_vao->Id(), 1);
    glVertexArrayAttribFormat(
        resources_.quad_vao->Id(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(resources_.quad_vao->Id(), 1, 0);
}

void AdvanceLightingState::CreateShadowMap()
{
    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.depth_map_texture);
    glTextureStorage2D(resources_.depth_map_texture,
                       1,
                       GL_DEPTH_COMPONENT24,
                       shadow_.shadow_width,
                       shadow_.shadow_height);
    glTextureParameteri(resources_.depth_map_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.depth_map_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.depth_map_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(resources_.depth_map_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    const float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(resources_.depth_map_texture, GL_TEXTURE_BORDER_COLOR, border_color);

    glCreateFramebuffers(1, &resources_.depth_map_fbo);
    glNamedFramebufferTexture(
        resources_.depth_map_fbo, GL_DEPTH_ATTACHMENT, resources_.depth_map_texture, 0);
    glNamedFramebufferDrawBuffer(resources_.depth_map_fbo, GL_NONE);
    glNamedFramebufferReadBuffer(resources_.depth_map_fbo, GL_NONE);
}

void AdvanceLightingState::DestroyShadowMap()
{
    if (resources_.depth_map_fbo) {
        glDeleteFramebuffers(1, &resources_.depth_map_fbo);
        resources_.depth_map_fbo = 0;
    }
    if (resources_.depth_map_texture) {
        glDeleteTextures(1, &resources_.depth_map_texture);
        resources_.depth_map_texture = 0;
    }
}

void AdvanceLightingState::CreateGBuffer(int width, int height)
{
    resources_.g_buffer_width = width;
    resources_.g_buffer_height = height;

    glCreateFramebuffers(1, &resources_.g_buffer_fbo);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_position_texture);
    glTextureStorage2D(resources_.g_position_texture, 1, GL_RGBA16F, width, height);
    glTextureParameteri(resources_.g_position_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_position_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_position_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.g_position_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT0, resources_.g_position_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_normal_texture);
    glTextureStorage2D(resources_.g_normal_texture, 1, GL_RGBA16F, width, height);
    glTextureParameteri(resources_.g_normal_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_normal_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_normal_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.g_normal_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT1, resources_.g_normal_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_albedo_spec_texture);
    glTextureStorage2D(resources_.g_albedo_spec_texture, 1, GL_RGBA8, width, height);
    glTextureParameteri(resources_.g_albedo_spec_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_albedo_spec_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(resources_.g_albedo_spec_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.g_albedo_spec_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT2, resources_.g_albedo_spec_texture, 0);

    glCreateRenderbuffers(1, &resources_.g_depth_rbo);
    glNamedRenderbufferStorage(resources_.g_depth_rbo, GL_DEPTH_COMPONENT24, width, height);
    glNamedFramebufferRenderbuffer(
        resources_.g_buffer_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, resources_.g_depth_rbo);

    GLenum attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glNamedFramebufferDrawBuffers(resources_.g_buffer_fbo, 3, attachments);
}

void AdvanceLightingState::DestroyGBuffer()
{
    if (resources_.g_depth_rbo) {
        glDeleteRenderbuffers(1, &resources_.g_depth_rbo);
        resources_.g_depth_rbo = 0;
    }
    if (resources_.g_albedo_spec_texture) {
        glDeleteTextures(1, &resources_.g_albedo_spec_texture);
        resources_.g_albedo_spec_texture = 0;
    }
    if (resources_.g_normal_texture) {
        glDeleteTextures(1, &resources_.g_normal_texture);
        resources_.g_normal_texture = 0;
    }
    if (resources_.g_position_texture) {
        glDeleteTextures(1, &resources_.g_position_texture);
        resources_.g_position_texture = 0;
    }
    if (resources_.g_buffer_fbo) {
        glDeleteFramebuffers(1, &resources_.g_buffer_fbo);
        resources_.g_buffer_fbo = 0;
    }

    resources_.g_buffer_width = 0;
    resources_.g_buffer_height = 0;
}

void AdvanceLightingState::SyncCamera(float width, float height)
{
    const float safe_height = std::max(height, 1.0f);
    camera_.camera.SetPosition(camera_.position);
    camera_.camera.SetRotation(camera_.pitch, camera_.yaw);
    camera_.camera.SetPerspective(camera_.fov, width / safe_height, 0.1f, 100.0f);
}

void AdvanceLightingState::EnsureGBuffer(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    if (!resources_.g_buffer_fbo || resources_.g_buffer_width != width ||
        resources_.g_buffer_height != height) {
        DestroyGBuffer();
        CreateGBuffer(width, height);
    }
}

void AdvanceLightingState::EnsureBloomBuffers(int width, int height, bool use_hdr_buffer)
{
    if (!use_hdr_buffer) {
        if (resources_.bloom_fbo) {
            DestroyBloomFBOs();
        }
        return;
    }

    if (width <= 0 || height <= 0) {
        return;
    }

    if (!resources_.bloom_fbo || resources_.bloom_fbo_width != width ||
        resources_.bloom_fbo_height != height) {
        DestroyBloomFBOs();
        CreateBloomFBOs(width, height);
    }
}

glm::mat4 AdvanceLightingState::BuildLightSpaceMatrix() const
{
    constexpr float near_plane = 1.0f;
    constexpr float far_plane = 7.5f;
    const AdvanceLightingPointLight& light = PrimaryLight(lighting_);

    const glm::mat4 light_projection =
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    const glm::mat4 light_view =
        glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return light_projection * light_view;
}

void AdvanceLightingState::RenderShadowPass(const glm::mat4& light_space_matrix)
{
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, shadow_.shadow_width, shadow_.shadow_height);
    glBindFramebuffer(GL_FRAMEBUFFER, resources_.depth_map_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    resources_.depth_shader->Bind();
    glUniformMatrix4fv(resources_.depth_shader->Uniform("u_LightSpaceMatrix"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(light_space_matrix));
    RenderScene(*resources_.depth_shader);
    resources_.depth_shader->Unbind();
}

void AdvanceLightingState::RenderGeometryPass(int width,
                                              int height,
                                              const glm::mat4& view,
                                              const glm::mat4& projection)
{
    static constexpr GLfloat kClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glBindFramebuffer(GL_FRAMEBUFFER, resources_.g_buffer_fbo);
    glViewport(0, 0, width, height);
    glClearBufferfv(GL_COLOR, 0, kClearColor);
    glClearBufferfv(GL_COLOR, 1, kClearColor);
    glClearBufferfv(GL_COLOR, 2, kClearColor);
    glClear(GL_DEPTH_BUFFER_BIT);

    resources_.geometry_shader->Bind();
    glUniformMatrix4fv(
        resources_.geometry_shader->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(resources_.geometry_shader->Uniform("u_projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(projection));

    glBindTextureUnit(0, resources_.floor_texture);
    glUniform1i(resources_.geometry_shader->Uniform("u_AlbedoTexture"), 0);

    RenderScene(*resources_.geometry_shader);
    resources_.geometry_shader->Unbind();
}

void AdvanceLightingState::RenderLightingPass(bool use_hdr_buffer,
                                              const std::array<GLint, 4>& viewport,
                                              GLint target_framebuffer,
                                              const glm::mat4& light_space_matrix)
{
    const int active_light_count =
        std::clamp(lighting_.active_light_count, 1, AdvanceLightingLightingSettings::kMaxLights);

    if (use_hdr_buffer && resources_.bloom_fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, resources_.bloom_fbo);
        glViewport(0, 0, resources_.bloom_fbo_width, resources_.bloom_fbo_height);
        glDisable(GL_FRAMEBUFFER_SRGB);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, target_framebuffer);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        if (post_process_.selected_gamma_correction == 1) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }
    }

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    resources_.lighting_shader->Bind();
    glUniformMatrix4fv(resources_.lighting_shader->Uniform("u_LightSpaceMatrix"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(light_space_matrix));
    glUniform3fv(
        resources_.lighting_shader->Uniform("u_ViewPos"), 1, glm::value_ptr(camera_.position));
    glUniform1f(resources_.lighting_shader->Uniform("u_Shininess"), lighting_.shininess);
    glUniform1f(resources_.lighting_shader->Uniform("u_AmbientStrength"),
                lighting_.ambient_strength);
    glUniform1f(resources_.lighting_shader->Uniform("u_SpecularStrength"),
                lighting_.specular_strength);
    glUniform1i(resources_.lighting_shader->Uniform("u_ActiveLightCount"), active_light_count);
    glUniform1i(resources_.lighting_shader->Uniform("u_BlinnPhong"),
                lighting_.use_blinn_phong ? 1 : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_UseGammaCorrection"),
                (!use_hdr_buffer && post_process_.selected_gamma_correction == 2) ? 1 : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_EnableShadows"),
                shadow_.enable_shadows ? 1 : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_PCFregionSize"), NormalizedPcfRegionSize());
    glUniform1f(resources_.lighting_shader->Uniform("u_BloomThreshold"),
                post_process_.enable_bloom ? post_process_.bloom_threshold : 99999.0f);

    for (int i = 0; i < active_light_count; ++i) {
        const auto& light = lighting_.lights[i];
        const std::string prefix = "u_Lights[" + std::to_string(i) + "].";
        glUniform3fv(resources_.lighting_shader->Uniform((prefix + "position").c_str()),
                     1,
                     glm::value_ptr(light.position));
        glUniform3fv(resources_.lighting_shader->Uniform((prefix + "color").c_str()),
                     1,
                     glm::value_ptr(light.color));
        glUniform1f(resources_.lighting_shader->Uniform((prefix + "intensity").c_str()),
                    light.intensity);
    }

    glBindTextureUnit(0, resources_.g_position_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GPosition"), 0);
    glBindTextureUnit(1, resources_.g_normal_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GNormal"), 1);
    glBindTextureUnit(2, resources_.g_albedo_spec_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GAlbedoSpec"), 2);
    glBindTextureUnit(3, resources_.depth_map_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_ShadowMap"), 3);

    RenderQuad();
    resources_.lighting_shader->Unbind();
}

void AdvanceLightingState::BlitGBufferDepth(bool use_hdr_buffer,
                                            const std::array<GLint, 4>& viewport,
                                            GLint target_framebuffer)
{
    GLuint draw_framebuffer = static_cast<GLuint>(target_framebuffer);
    GLint dst_x = viewport[0];
    GLint dst_y = viewport[1];
    GLint dst_width = viewport[2];
    GLint dst_height = viewport[3];

    if (use_hdr_buffer && resources_.bloom_fbo) {
        draw_framebuffer = resources_.bloom_fbo;
        dst_x = 0;
        dst_y = 0;
        dst_width = resources_.bloom_fbo_width;
        dst_height = resources_.bloom_fbo_height;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, resources_.g_buffer_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer);
    glBlitFramebuffer(0,
                      0,
                      resources_.g_buffer_width,
                      resources_.g_buffer_height,
                      dst_x,
                      dst_y,
                      dst_x + dst_width,
                      dst_y + dst_height,
                      GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, draw_framebuffer);
    glViewport(dst_x, dst_y, dst_width, dst_height);
}

void AdvanceLightingState::RenderLightCube(bool use_hdr_buffer,
                                           const glm::mat4& view,
                                           const glm::mat4& projection)
{
    const int active_light_count =
        std::clamp(lighting_.active_light_count, 1, AdvanceLightingLightingSettings::kMaxLights);

    glEnable(GL_DEPTH_TEST);
    resources_.lamp_shader->Bind();
    glUniformMatrix4fv(
        resources_.lamp_shader->Uniform("u_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
        resources_.lamp_shader->Uniform("u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(resources_.lamp_shader->Uniform("u_UseGammaCorrection"),
                (!use_hdr_buffer && post_process_.selected_gamma_correction == 2) ? 1 : 0);
    resources_.cube_vao->Bind();
    for (int i = 0; i < active_light_count; ++i) {
        const auto& light = lighting_.lights[i];
        glm::mat4 model(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(i == 0 ? 0.22f : 0.16f));

        const glm::vec3 hdr_light_color = HdrLightColor(light);
        glUniformMatrix4fv(
            resources_.lamp_shader->Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(
            resources_.lamp_shader->Uniform("u_LightColor"), 1, glm::value_ptr(hdr_light_color));
        glUniform1f(resources_.lamp_shader->Uniform("u_BloomThreshold"),
                    post_process_.enable_bloom ? post_process_.bloom_threshold : 99999.0f);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    resources_.cube_vao->Unbind();
    resources_.lamp_shader->Unbind();
}

void AdvanceLightingState::RenderPostProcess(const std::array<GLint, 4>& viewport,
                                             GLint target_framebuffer)
{
    if (!resources_.bloom_fbo || !resources_.bloom_composite_shader) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);

    GLuint bloom_blurred_texture = resources_.bloom_color_textures[1];
    bool horizontal = true;

    if (post_process_.enable_bloom && resources_.bloom_blur_shader) {
        bool first_iteration = true;
        const int amount = post_process_.bloom_blur_iterations * 2;

        resources_.bloom_blur_shader->Bind();
        for (int i = 0; i < amount; ++i) {
            const int write_index = horizontal ? 1 : 0;
            const int read_index = horizontal ? 0 : 1;

            glBindFramebuffer(GL_FRAMEBUFFER, resources_.bloom_ping_pong_fbos[write_index]);
            glUniform1i(resources_.bloom_blur_shader->Uniform("u_Horizontal"), horizontal ? 1 : 0);
            glBindTextureUnit(0,
                              first_iteration ? resources_.bloom_color_textures[1]
                                              : resources_.bloom_ping_pong_textures[read_index]);
            glUniform1i(resources_.bloom_blur_shader->Uniform("u_Image"), 0);

            RenderQuad();
            horizontal = !horizontal;
            first_iteration = false;
        }
        resources_.bloom_blur_shader->Unbind();
        bloom_blurred_texture = resources_.bloom_ping_pong_textures[horizontal ? 0 : 1];
    }

    glBindFramebuffer(GL_FRAMEBUFFER, target_framebuffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (post_process_.selected_gamma_correction == 1) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    resources_.bloom_composite_shader->Bind();
    glBindTextureUnit(0, resources_.bloom_color_textures[0]);
    glUniform1i(resources_.bloom_composite_shader->Uniform("u_SceneColor"), 0);
    glBindTextureUnit(1, bloom_blurred_texture);
    glUniform1i(resources_.bloom_composite_shader->Uniform("u_BloomBlur"), 1);

    glUniform1i(resources_.bloom_composite_shader->Uniform("u_EnableBloom"),
                post_process_.enable_bloom ? 1 : 0);
    glUniform1f(resources_.bloom_composite_shader->Uniform("u_BloomIntensity"),
                post_process_.bloom_intensity);
    glUniform1i(resources_.bloom_composite_shader->Uniform("u_EnableHDR"),
                post_process_.enable_hdr ? 1 : 0);
    glUniform1i(resources_.bloom_composite_shader->Uniform("u_ToneMappingMode"),
                post_process_.tone_mapping_mode);
    glUniform1f(resources_.bloom_composite_shader->Uniform("u_Exposure"), post_process_.exposure);
    glUniform1i(resources_.bloom_composite_shader->Uniform("u_ApplyGamma"),
                post_process_.selected_gamma_correction == 2 ? 1 : 0);

    RenderQuad();
    resources_.bloom_composite_shader->Unbind();

    glEnable(GL_DEPTH_TEST);
}

void AdvanceLightingState::RenderScene(gl::Shader& shader)
{
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    resources_.floor_vao->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    resources_.floor_vao->Unbind();

    resources_.scene_cube_vao->Bind();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.25f, 1.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.5f, 2.0f));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(shader.Uniform("u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    resources_.scene_cube_vao->Unbind();
}

void AdvanceLightingState::RenderQuad()
{
    resources_.quad_vao->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    resources_.quad_vao->Unbind();
}

void AdvanceLightingState::CreateBloomFBOs(int width, int height)
{
    resources_.bloom_fbo_width = width;
    resources_.bloom_fbo_height = height;

    glCreateFramebuffers(1, &resources_.bloom_fbo);

    for (int i = 0; i < 2; ++i) {
        glCreateTextures(GL_TEXTURE_2D, 1, &resources_.bloom_color_textures[i]);
        glTextureStorage2D(resources_.bloom_color_textures[i], 1, GL_RGBA16F, width, height);
        glTextureParameteri(resources_.bloom_color_textures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(resources_.bloom_color_textures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(
            resources_.bloom_color_textures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(
            resources_.bloom_color_textures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(
            resources_.bloom_fbo, GL_COLOR_ATTACHMENT0 + i, resources_.bloom_color_textures[i], 0);
    }

    glCreateRenderbuffers(1, &resources_.bloom_rbo);
    glNamedRenderbufferStorage(resources_.bloom_rbo, GL_DEPTH24_STENCIL8, width, height);
    glNamedFramebufferRenderbuffer(
        resources_.bloom_fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, resources_.bloom_rbo);

    GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glNamedFramebufferDrawBuffers(resources_.bloom_fbo, 2, attachments);

    for (int i = 0; i < 2; ++i) {
        glCreateFramebuffers(1, &resources_.bloom_ping_pong_fbos[i]);
        glCreateTextures(GL_TEXTURE_2D, 1, &resources_.bloom_ping_pong_textures[i]);
        glTextureStorage2D(resources_.bloom_ping_pong_textures[i], 1, GL_RGBA16F, width, height);
        glTextureParameteri(
            resources_.bloom_ping_pong_textures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(
            resources_.bloom_ping_pong_textures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(
            resources_.bloom_ping_pong_textures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(
            resources_.bloom_ping_pong_textures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(resources_.bloom_ping_pong_fbos[i],
                                  GL_COLOR_ATTACHMENT0,
                                  resources_.bloom_ping_pong_textures[i],
                                  0);
    }
}

void AdvanceLightingState::DestroyBloomFBOs()
{
    for (int i = 0; i < 2; ++i) {
        if (resources_.bloom_ping_pong_textures[i]) {
            glDeleteTextures(1, &resources_.bloom_ping_pong_textures[i]);
            resources_.bloom_ping_pong_textures[i] = 0;
        }
        if (resources_.bloom_ping_pong_fbos[i]) {
            glDeleteFramebuffers(1, &resources_.bloom_ping_pong_fbos[i]);
            resources_.bloom_ping_pong_fbos[i] = 0;
        }
        if (resources_.bloom_color_textures[i]) {
            glDeleteTextures(1, &resources_.bloom_color_textures[i]);
            resources_.bloom_color_textures[i] = 0;
        }
    }

    if (resources_.bloom_rbo) {
        glDeleteRenderbuffers(1, &resources_.bloom_rbo);
        resources_.bloom_rbo = 0;
    }
    if (resources_.bloom_fbo) {
        glDeleteFramebuffers(1, &resources_.bloom_fbo);
        resources_.bloom_fbo = 0;
    }

    resources_.bloom_fbo_width = 0;
    resources_.bloom_fbo_height = 0;
}

GLuint AdvanceLightingState::LoadTexture(const char* path) const
{
    stbi_set_flip_vertically_on_load(true);

    int tex_width = 0;
    int tex_height = 0;
    int tex_channels = 0;
    unsigned char* data = stbi_load(path, &tex_width, &tex_height, &tex_channels, 0);
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

    int max_dim = std::max(tex_width, tex_height);
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
    glTextureStorage2D(texture, levels, internal_format, tex_width, tex_height);
    glTextureSubImage2D(texture, 0, 0, 0, tex_width, tex_height, format, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(texture);

    stbi_image_free(data);
    return texture;
}

int AdvanceLightingState::NormalizedPcfRegionSize() const
{
    const int clamped = std::max(shadow_.pcf_region_size, 1);
    return (clamped % 2 == 0) ? clamped + 1 : clamped;
}

bool AdvanceLightingState::OnKeyPressed(int keycode)
{
    if (keycode == GLFW_KEY_B) {
        lighting_.use_blinn_phong = !lighting_.use_blinn_phong;
        return true;
    }

    input_.keys_down.insert(keycode);
    return false;
}

bool AdvanceLightingState::OnKeyReleased(int keycode)
{
    input_.keys_down.erase(keycode);
    return false;
}

bool AdvanceLightingState::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        input_.right_mouse_down = true;
        input_.first_mouse = true;
        return true;
    }
    return false;
}

bool AdvanceLightingState::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        input_.right_mouse_down = false;
        return true;
    }
    return false;
}

bool AdvanceLightingState::OnMouseMoved(float x, float y)
{
    if (!input_.right_mouse_down) {
        return false;
    }

    if (input_.first_mouse) {
        input_.last_mouse_x = x;
        input_.last_mouse_y = y;
        input_.first_mouse = false;
        return true;
    }

    const float dx = x - input_.last_mouse_x;
    const float dy = input_.last_mouse_y - y;
    input_.last_mouse_x = x;
    input_.last_mouse_y = y;

    camera_.yaw += dx * camera_.mouse_sensitivity;
    camera_.pitch += dy * camera_.mouse_sensitivity;
    camera_.pitch = glm::clamp(camera_.pitch, -89.0f, 89.0f);
    return true;
}

bool AdvanceLightingState::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    camera_.fov -= y_offset * 2.0f;
    camera_.fov = glm::clamp(camera_.fov, 10.0f, 120.0f);
    return true;
}
