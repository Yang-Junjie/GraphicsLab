#include "Scene/Scenes/PBR/PBRState.hpp"

#include <cctype>

#include <algorithm>
#include <array>
#include <Event.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kModelRoot = "res/model";
constexpr const char* kHdrRoot = "res/hdr";

constexpr int kEnvironmentMapSize = 512;
constexpr int kIrradianceMapSize = 32;
constexpr int kPrefilterMapSize = 128;
constexpr int kPrefilterMipLevels = 5;
constexpr int kBrdfLutSize = 512;

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

void RestoreFramebufferState(const FramebufferState& state)
{
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(state.framebuffer));
    glViewport(state.viewport[0], state.viewport[1], state.viewport[2], state.viewport[3]);
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool IsModelAsset(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    return extension == ".gltf" || extension == ".glb";
}

bool IsHdrAsset(const std::filesystem::path& path)
{
    return ToLower(path.extension().string()) == ".hdr";
}

std::filesystem::path ResolveConfiguredModelPath(const std::filesystem::path& model_path)
{
    if (model_path.empty()) {
        return {};
    }

    if (std::filesystem::exists(model_path)) {
        return model_path;
    }

    const std::filesystem::path rooted_model_path = std::filesystem::path(kModelRoot) / model_path;
    if (std::filesystem::exists(rooted_model_path)) {
        return rooted_model_path;
    }

    return model_path;
}

std::filesystem::path FindFirstModelPath(const std::filesystem::path& root)
{
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return {};
    }

    std::vector<std::filesystem::path> candidates;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_regular_file() && IsModelAsset(entry.path())) {
                candidates.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return {};
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.generic_string() < rhs.generic_string();
    });

    if (candidates.empty()) {
        return {};
    }

    return candidates.front();
}

std::filesystem::path FindFirstHdrPath(const std::filesystem::path& root)
{
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return {};
    }

    std::vector<std::filesystem::path> candidates;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (entry.is_regular_file() && IsHdrAsset(entry.path())) {
                candidates.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return {};
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.generic_string() < rhs.generic_string();
    });

    if (candidates.empty()) {
        return {};
    }

    return candidates.front();
}

int CalculateMipLevels(int size)
{
    int mip_levels = 1;
    while (size > 1) {
        size /= 2;
        ++mip_levels;
    }
    return mip_levels;
}

std::array<glm::mat4, 6> BuildCaptureViews()
{
    return {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };
}

bool IsFramebufferComplete(GLuint framebuffer)
{
    return glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

} // namespace

PBRState::PBRState(std::filesystem::path model_path)
    : configured_model_path_(std::move(model_path))
{}

void PBRState::OnEnter()
{
    hdr_status_.clear();
    ibl_ready_ = false;

    ready_ = CreateShaders();
    CreateGeometry();
    CreateImageBasedLighting();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    camera_.camera.SetPosition(camera_.position);
    camera_.camera.SetRotation(camera_.pitch, camera_.yaw);
}

void PBRState::OnExit()
{
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);

    DestroyGBuffer();
    DestroyImageBasedLighting();

    resources_.cube_vbo.reset();
    resources_.quad_vbo.reset();

    resources_.model.reset();
    resources_.cube_vao.reset();
    resources_.quad_vao.reset();

    resources_.brdf_lut_shader.reset();
    resources_.prefilter_shader.reset();
    resources_.irradiance_shader.reset();
    resources_.equirect_to_cubemap_shader.reset();
    resources_.lighting_shader.reset();
    resources_.geometry_shader.reset();

    input_.keys_down.clear();
    input_.right_mouse_down = false;
    input_.first_mouse = true;
    model_path_.clear();
    hdr_path_.clear();
    model_status_.clear();
    hdr_status_.clear();
    ibl_ready_ = false;
    ready_ = false;
}

void PBRState::OnUpdate(float dt)
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

void PBRState::OnRender(float width, float height)
{
    if (!ready_ || !resources_.geometry_shader || !resources_.lighting_shader ||
        !resources_.quad_vao) {
        return;
    }

    const int viewport_width = std::max(static_cast<int>(width), 0);
    const int viewport_height = std::max(static_cast<int>(height), 0);
    if (viewport_width <= 0 || viewport_height <= 0) {
        return;
    }

    SyncCamera(width, height);
    EnsureGBuffer(viewport_width, viewport_height);
    if (!resources_.g_buffer_fbo) {
        return;
    }

    const FramebufferState framebuffer_state = CaptureFramebufferState();
    const glm::mat4 view = camera_.camera.GetViewMatrix();
    const glm::mat4 projection = camera_.camera.GetProjectionMatrix();

    RenderGeometryPass(viewport_width, viewport_height, view, projection);
    RenderLightingPass(framebuffer_state.viewport, framebuffer_state.framebuffer, view, projection);
}

void PBRState::OnRenderUI()
{
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
                preview_texture = resources_.g_albedo_texture;
                break;
            case 3:
                preview_texture = resources_.g_metallic_texture;
                break;
            case 4:
                preview_texture = resources_.g_roughness_texture;
                break;
            case 5:
                preview_texture = resources_.g_ao_texture;
                break;
            case 6:
                preview_texture = resources_.g_emissive_texture;
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

    ImGui::Text("Deferred PBR");
    ImGui::Separator();

    ImGui::TextWrapped("Model: %s", model_status_.c_str());
    if (!model_path_.empty()) {
        ImGui::TextWrapped("Asset: %s", model_path_.generic_string().c_str());
    }
    if (resources_.model && resources_.model->IsLoaded()) {
        ImGui::Text("Meshes: %llu | Materials: %llu | Instances: %llu",
                    static_cast<unsigned long long>(resources_.model->GetMeshCount()),
                    static_cast<unsigned long long>(resources_.model->GetMaterialCount()),
                    static_cast<unsigned long long>(resources_.model->GetInstanceCount()));
    }

    ImGui::Separator();
    ImGui::Text("Environment");
    ImGui::TextWrapped("IBL: %s", hdr_status_.empty() ? "Unavailable" : hdr_status_.c_str());
    if (!hdr_path_.empty()) {
        ImGui::TextWrapped("HDR: %s", hdr_path_.generic_string().c_str());
    }
    ImGui::Checkbox("Show Environment", &lighting_.show_environment);
    ImGui::SliderFloat("IBL Intensity", &lighting_.ibl_intensity, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat(
        "Environment Intensity", &lighting_.environment_intensity, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat("Exposure", &lighting_.exposure, 0.1f, 5.0f, "%.2f");
    ImGui::SliderFloat("Background Blur",
                       &lighting_.background_lod,
                       0.0f,
                       static_cast<float>(CalculateMipLevels(kEnvironmentMapSize) - 1),
                       "%.2f");
    ImGui::SliderFloat("Fallback Ambient", &lighting_.ambient_intensity, 0.0f, 0.2f, "%.3f");
    ImGui::ColorEdit3("Fallback Background", &lighting_.background_color.x);

    const char* lighting_outputs[] = {"Final", "IBL Diffuse", "IBL Prefilter", "IBL BRDF", "IBL Specular", "IBL Ambient"};
    ImGui::Combo("Lighting Output", &debug_.lighting_output_mode, lighting_outputs, 6);

    ImGui::Separator();
    ImGui::Text("Material");
    ImGui::TextWrapped("Base color, normal, occlusion, roughness, metallic, and emissive come from "
                       "the glTF material textures.");
    ImGui::SliderFloat("Model Scale", &material_.model_scale, 0.05f, 4.0f, "%.2f");

    ImGui::Separator();
    ImGui::Text("Light");
    ImGui::DragFloat3("Light Position", &lighting_.light.position.x, 0.05f);
    ImGui::ColorEdit3("Light Color", &lighting_.light.color.x);
    ImGui::SliderFloat("Light Intensity", &lighting_.light.intensity, 0.0f, 120.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::DragFloat3("Position", &camera_.position.x, 0.05f);
    ImGui::SliderFloat("Pitch", &camera_.pitch, -89.0f, 89.0f);
    ImGui::SliderFloat("Yaw", &camera_.yaw, -180.0f, 360.0f);
    ImGui::SliderFloat("FOV", &camera_.fov, 10.0f, 120.0f);
    ImGui::SliderFloat("Move Speed", &camera_.move_speed, 1.0f, 20.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera_.mouse_sensitivity, 0.01f, 1.0f);

    ImGui::Separator();
    ImGui::Text("G-Buffer Preview");
    const char* gbuffer_previews[] = {
        "Position", "Normal", "Albedo", "Metallic", "Roughness", "AO", "Emissive"};
    ImGui::Combo("Attachment", &debug_.selected_gbuffer_preview, gbuffer_previews, 7);
    switch (debug_.selected_gbuffer_preview) {
        case 0:
            render_preview(
                "Position buffer stores world-space coordinates, so the preview is unclamped.");
            break;
        case 1:
            render_preview("Normal buffer stores encoded normals only.");
            break;
        case 2:
            render_preview("Albedo buffer stores glTF base color.");
            break;
        case 3:
            render_preview("Metallic comes from the glTF metallic-roughness texture B channel.");
            break;
        case 4:
            render_preview("Roughness comes from the glTF metallic-roughness texture G channel.");
            break;
        case 5:
            render_preview("AO comes from the glTF occlusion texture R channel.");
            break;
        case 6:
            render_preview("Emissive stores the glTF emissive contribution.");
            break;
        default:
            break;
    }

    ImGui::Separator();
    ImGui::TextWrapped("Deferred: Geometry Pass + Lighting Pass + IBL\n"
                       "WASD/Arrow: Move, Space/Shift: Up/Down\n"
                       "Scroll: FOV, RMB Drag: Look");
}

void PBRState::OnEvent(flux::Event& event)
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

bool PBRState::CreateShaders()
{
    bool ok = true;

    resources_.geometry_shader = std::make_unique<gl::Shader>();
    ok = resources_.geometry_shader->CompileFromFile("shaders/PBR/geometry.vert",
                                                     "shaders/PBR/geometry.frag") &&
         ok;

    resources_.lighting_shader = std::make_unique<gl::Shader>();
    ok = resources_.lighting_shader->CompileFromFile("shaders/PBR/fullscreen.vert",
                                                     "shaders/PBR/lighting.frag") &&
         ok;

    bool ibl_ok = true;

    resources_.equirect_to_cubemap_shader = std::make_unique<gl::Shader>();
    ibl_ok = resources_.equirect_to_cubemap_shader->CompileFromFile(
                 "shaders/PBR/cube.vert", "shaders/PBR/equirect_to_cubemap.frag") &&
             ibl_ok;

    resources_.irradiance_shader = std::make_unique<gl::Shader>();
    ibl_ok = resources_.irradiance_shader->CompileFromFile(
                 "shaders/PBR/cube.vert", "shaders/PBR/irradiance_convolution.frag") &&
             ibl_ok;

    resources_.prefilter_shader = std::make_unique<gl::Shader>();
    ibl_ok = resources_.prefilter_shader->CompileFromFile("shaders/PBR/cube.vert",
                                                          "shaders/PBR/prefilter.frag") &&
             ibl_ok;

    resources_.brdf_lut_shader = std::make_unique<gl::Shader>();
    ibl_ok = resources_.brdf_lut_shader->CompileFromFile("shaders/PBR/fullscreen.vert",
                                                         "shaders/PBR/brdf_lut.frag") &&
             ibl_ok;

    if (!ibl_ok) {
        hdr_status_ = "IBL shader compile failed";
    }

    return ok;
}

void PBRState::CreateGeometry()
{
    LoadModel();
    CreateQuadGeometry();
    CreateCubeGeometry();
}

bool PBRState::LoadModel()
{
    resources_.model.reset();
    model_path_.clear();

    const std::filesystem::path model_path =
        configured_model_path_.empty() ? FindFirstModelPath(kModelRoot)
                                       : ResolveConfiguredModelPath(configured_model_path_);
    if (model_path.empty()) {
        model_status_ = "No .gltf or .glb asset found under res/model";
        return false;
    }

    if (!std::filesystem::exists(model_path) || !std::filesystem::is_regular_file(model_path)) {
        model_status_ = "Configured model path not found";
        model_path_ = model_path;
        return false;
    }

    if (!IsModelAsset(model_path)) {
        model_status_ = "Configured asset must be a .gltf or .glb file";
        model_path_ = model_path;
        return false;
    }

    auto model = std::make_unique<Model>();
    if (!model->LoadFromGLTF(model_path)) {
        model_status_ = "Model load failed";
        model_path_ = model_path;
        return false;
    }

    model_path_ = model_path;
    model_status_ = "Model loaded";
    resources_.model = std::move(model);
    return true;
}

void PBRState::CreateQuadGeometry()
{
    static constexpr float quad_vertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
    };

    resources_.quad_vao = std::make_unique<gl::VertexArray>();
    resources_.quad_vbo = std::make_unique<gl::Buffer>();
    resources_.quad_vbo->Storage(sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    const GLuint vao = resources_.quad_vao->Id();
    glVertexArrayVertexBuffer(vao, 0, resources_.quad_vbo->Id(), 0, 4 * sizeof(float));

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao, 1, 0);
}

void PBRState::CreateCubeGeometry()
{
    static constexpr float cube_vertices[] = {
        -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    };

    resources_.cube_vao = std::make_unique<gl::VertexArray>();
    resources_.cube_vbo = std::make_unique<gl::Buffer>();
    resources_.cube_vbo->Storage(sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    const GLuint vao = resources_.cube_vao->Id();
    glVertexArrayVertexBuffer(vao, 0, resources_.cube_vbo->Id(), 0, 3 * sizeof(float));

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
}

bool PBRState::CreateImageBasedLighting()
{
    DestroyImageBasedLighting();
    hdr_path_.clear();
    ibl_ready_ = false;

    if (!resources_.quad_vao || !resources_.cube_vao || !resources_.quad_vbo ||
        !resources_.cube_vbo || !resources_.equirect_to_cubemap_shader ||
        !resources_.irradiance_shader || !resources_.prefilter_shader ||
        !resources_.brdf_lut_shader || resources_.equirect_to_cubemap_shader->Id() == 0 ||
        resources_.irradiance_shader->Id() == 0 || resources_.prefilter_shader->Id() == 0 ||
        resources_.brdf_lut_shader->Id() == 0) {
        if (hdr_status_.empty()) {
            hdr_status_ = "IBL shaders unavailable";
        }
        return false;
    }

    const std::filesystem::path hdr_path = FindFirstHdrPath(kHdrRoot);
    if (hdr_path.empty()) {
        hdr_status_ = "No .hdr asset found under res/hdr";
        return false;
    }

    hdr_path_ = hdr_path;
    if (!std::filesystem::exists(hdr_path_) || !std::filesystem::is_regular_file(hdr_path_)) {
        hdr_status_ = "HDR asset not found";
        return false;
    }

    auto create_cubemap = [](GLuint& texture, int size, int levels, GLenum internal_format) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
        glTextureStorage2D(texture, levels, internal_format, size, size);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(
            texture, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, levels - 1);
    };

    const FramebufferState framebuffer_state = CaptureFramebufferState();
    const auto restore_state = [&framebuffer_state]() {
        RestoreFramebufferState(framebuffer_state);
    };

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    const std::string hdr_path_string = hdr_path_.string();
    float* pixels = stbi_loadf(hdr_path_string.c_str(), &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(false);
    if (!pixels || width <= 0 || height <= 0) {
        if (pixels) {
            stbi_image_free(pixels);
        }
        hdr_status_ = "HDR load failed";
        return false;
    }

    GLenum format = GL_RGB;
    GLenum internal_format = GL_RGB16F;
    if (channels == 1) {
        format = GL_RED;
        internal_format = GL_R16F;
    } else if (channels == 2) {
        format = GL_RG;
        internal_format = GL_RG16F;
    } else if (channels == 4) {
        format = GL_RGBA;
        internal_format = GL_RGBA16F;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.hdr_texture);
    glTextureStorage2D(resources_.hdr_texture, 1, internal_format, width, height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTextureSubImage2D(resources_.hdr_texture, 0, 0, 0, width, height, format, GL_FLOAT, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTextureParameteri(resources_.hdr_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.hdr_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.hdr_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(resources_.hdr_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(pixels);

    glCreateFramebuffers(1, &resources_.capture_fbo);
    glCreateRenderbuffers(1, &resources_.capture_rbo);
    glNamedFramebufferRenderbuffer(
        resources_.capture_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, resources_.capture_rbo);
    constexpr GLenum color_attachment = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(resources_.capture_fbo, 1, &color_attachment);

    create_cubemap(resources_.environment_cubemap,
                   kEnvironmentMapSize,
                   CalculateMipLevels(kEnvironmentMapSize),
                   GL_RGB16F);
    create_cubemap(resources_.irradiance_cubemap, kIrradianceMapSize, 1, GL_RGB16F);
    create_cubemap(resources_.prefilter_cubemap, kPrefilterMapSize, kPrefilterMipLevels, GL_RGB16F);

    resources_.prefilter_mip_levels = kPrefilterMipLevels;

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.brdf_lut_texture);
    glTextureStorage2D(resources_.brdf_lut_texture, 1, GL_RGBA16F, kBrdfLutSize, kBrdfLutSize);
    glTextureParameteri(resources_.brdf_lut_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.brdf_lut_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resources_.brdf_lut_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(resources_.brdf_lut_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const auto capture_views = BuildCaptureViews();
    static constexpr GLfloat clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, resources_.capture_fbo);

    glNamedRenderbufferStorage(
        resources_.capture_rbo, GL_DEPTH_COMPONENT24, kEnvironmentMapSize, kEnvironmentMapSize);
    resources_.equirect_to_cubemap_shader->Bind();
    glUniform1i(resources_.equirect_to_cubemap_shader->Uniform("u_EquirectangularMap"), 0);
    glUniformMatrix4fv(resources_.equirect_to_cubemap_shader->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(capture_projection));
    glBindTextureUnit(0, resources_.hdr_texture);
    for (int face = 0; face < 6; ++face) {
        glUniformMatrix4fv(resources_.equirect_to_cubemap_shader->Uniform("u_View"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(capture_views[face]));
        glNamedFramebufferTextureLayer(
            resources_.capture_fbo, GL_COLOR_ATTACHMENT0, resources_.environment_cubemap, 0, face);
        if (!IsFramebufferComplete(resources_.capture_fbo)) {
            resources_.equirect_to_cubemap_shader->Unbind();
            restore_state();
            DestroyImageBasedLighting();
            hdr_status_ = "Environment cubemap framebuffer incomplete";
            return false;
        }

        glViewport(0, 0, kEnvironmentMapSize, kEnvironmentMapSize);
        glClearBufferfv(GL_COLOR, 0, clear_color);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }
    resources_.equirect_to_cubemap_shader->Unbind();
    glGenerateTextureMipmap(resources_.environment_cubemap);

    glNamedRenderbufferStorage(
        resources_.capture_rbo, GL_DEPTH_COMPONENT24, kIrradianceMapSize, kIrradianceMapSize);
    resources_.irradiance_shader->Bind();
    glUniform1i(resources_.irradiance_shader->Uniform("u_EnvironmentMap"), 0);
    glUniformMatrix4fv(resources_.irradiance_shader->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(capture_projection));
    glBindTextureUnit(0, resources_.environment_cubemap);
    for (int face = 0; face < 6; ++face) {
        glUniformMatrix4fv(resources_.irradiance_shader->Uniform("u_View"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(capture_views[face]));
        glNamedFramebufferTextureLayer(
            resources_.capture_fbo, GL_COLOR_ATTACHMENT0, resources_.irradiance_cubemap, 0, face);
        if (!IsFramebufferComplete(resources_.capture_fbo)) {
            resources_.irradiance_shader->Unbind();
            restore_state();
            DestroyImageBasedLighting();
            hdr_status_ = "Irradiance framebuffer incomplete";
            return false;
        }

        glViewport(0, 0, kIrradianceMapSize, kIrradianceMapSize);
        glClearBufferfv(GL_COLOR, 0, clear_color);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }
    resources_.irradiance_shader->Unbind();

    resources_.prefilter_shader->Bind();
    glUniform1i(resources_.prefilter_shader->Uniform("u_EnvironmentMap"), 0);
    glUniformMatrix4fv(resources_.prefilter_shader->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(capture_projection));
    glBindTextureUnit(0, resources_.environment_cubemap);
    for (int mip = 0; mip < resources_.prefilter_mip_levels; ++mip) {
        const int mip_size = std::max(1, kPrefilterMapSize >> mip);
        const float roughness =
            resources_.prefilter_mip_levels > 1
                ? static_cast<float>(mip) / static_cast<float>(resources_.prefilter_mip_levels - 1)
                : 0.0f;

        glNamedRenderbufferStorage(
            resources_.capture_rbo, GL_DEPTH_COMPONENT24, mip_size, mip_size);
        glViewport(0, 0, mip_size, mip_size);
        glUniform1f(resources_.prefilter_shader->Uniform("u_Roughness"), roughness);

        for (int face = 0; face < 6; ++face) {
            glUniformMatrix4fv(resources_.prefilter_shader->Uniform("u_View"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(capture_views[face]));
            glNamedFramebufferTextureLayer(resources_.capture_fbo,
                                           GL_COLOR_ATTACHMENT0,
                                           resources_.prefilter_cubemap,
                                           mip,
                                           face);
            if (!IsFramebufferComplete(resources_.capture_fbo)) {
                resources_.prefilter_shader->Unbind();
                restore_state();
                DestroyImageBasedLighting();
                hdr_status_ = "Prefilter framebuffer incomplete";
                return false;
            }

            glClearBufferfv(GL_COLOR, 0, clear_color);
            glClear(GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }
    resources_.prefilter_shader->Unbind();

    glNamedRenderbufferStorage(
        resources_.capture_rbo, GL_DEPTH_COMPONENT24, kBrdfLutSize, kBrdfLutSize);
    glNamedFramebufferTexture(
        resources_.capture_fbo, GL_COLOR_ATTACHMENT0, resources_.brdf_lut_texture, 0);
    if (!IsFramebufferComplete(resources_.capture_fbo)) {
        restore_state();
        DestroyImageBasedLighting();
        hdr_status_ = "BRDF LUT framebuffer incomplete";
        return false;
    }

    glViewport(0, 0, kBrdfLutSize, kBrdfLutSize);
    glClearBufferfv(GL_COLOR, 0, clear_color);
    glClear(GL_DEPTH_BUFFER_BIT);
    resources_.brdf_lut_shader->Bind();
    RenderQuad();
    resources_.brdf_lut_shader->Unbind();

    restore_state();
    hdr_status_ = "HDR + IBL ready";
    ibl_ready_ = true;
    return true;
}

void PBRState::CreateGBuffer(int width, int height)
{
    resources_.g_buffer_width = width;
    resources_.g_buffer_height = height;

    glCreateFramebuffers(1, &resources_.g_buffer_fbo);

    auto setup_color_texture = [width, height](GLuint texture, GLenum internal_format) {
        glTextureStorage2D(texture, 1, internal_format, width, height);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    auto setup_scalar_preview_swizzle = [](GLuint texture) {
        glTextureParameteri(texture, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTextureParameteri(texture, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTextureParameteri(texture, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTextureParameteri(texture, GL_TEXTURE_SWIZZLE_A, GL_ONE);
    };

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_position_texture);
    setup_color_texture(resources_.g_position_texture, GL_RGBA16F);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT0, resources_.g_position_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_normal_texture);
    setup_color_texture(resources_.g_normal_texture, GL_RGBA16F);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT1, resources_.g_normal_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_albedo_texture);
    setup_color_texture(resources_.g_albedo_texture, GL_RGBA8);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT2, resources_.g_albedo_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_metallic_texture);
    setup_color_texture(resources_.g_metallic_texture, GL_R16F);
    setup_scalar_preview_swizzle(resources_.g_metallic_texture);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT3, resources_.g_metallic_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_roughness_texture);
    setup_color_texture(resources_.g_roughness_texture, GL_R16F);
    setup_scalar_preview_swizzle(resources_.g_roughness_texture);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT4, resources_.g_roughness_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_ao_texture);
    setup_color_texture(resources_.g_ao_texture, GL_R16F);
    setup_scalar_preview_swizzle(resources_.g_ao_texture);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT5, resources_.g_ao_texture, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &resources_.g_emissive_texture);
    setup_color_texture(resources_.g_emissive_texture, GL_RGBA16F);
    glNamedFramebufferTexture(
        resources_.g_buffer_fbo, GL_COLOR_ATTACHMENT6, resources_.g_emissive_texture, 0);

    glCreateRenderbuffers(1, &resources_.g_depth_rbo);
    glNamedRenderbufferStorage(resources_.g_depth_rbo, GL_DEPTH_COMPONENT24, width, height);
    glNamedFramebufferRenderbuffer(
        resources_.g_buffer_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, resources_.g_depth_rbo);

    constexpr GLenum attachments[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
    };
    glNamedFramebufferDrawBuffers(resources_.g_buffer_fbo, 7, attachments);

    if (!IsFramebufferComplete(resources_.g_buffer_fbo)) {
        DestroyGBuffer();
    }
}

void PBRState::DestroyGBuffer()
{
    if (resources_.g_depth_rbo) {
        glDeleteRenderbuffers(1, &resources_.g_depth_rbo);
        resources_.g_depth_rbo = 0;
    }
    if (resources_.g_emissive_texture) {
        glDeleteTextures(1, &resources_.g_emissive_texture);
        resources_.g_emissive_texture = 0;
    }
    if (resources_.g_ao_texture) {
        glDeleteTextures(1, &resources_.g_ao_texture);
        resources_.g_ao_texture = 0;
    }
    if (resources_.g_roughness_texture) {
        glDeleteTextures(1, &resources_.g_roughness_texture);
        resources_.g_roughness_texture = 0;
    }
    if (resources_.g_metallic_texture) {
        glDeleteTextures(1, &resources_.g_metallic_texture);
        resources_.g_metallic_texture = 0;
    }
    if (resources_.g_albedo_texture) {
        glDeleteTextures(1, &resources_.g_albedo_texture);
        resources_.g_albedo_texture = 0;
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

void PBRState::DestroyImageBasedLighting()
{
    if (resources_.brdf_lut_texture) {
        glDeleteTextures(1, &resources_.brdf_lut_texture);
        resources_.brdf_lut_texture = 0;
    }
    if (resources_.prefilter_cubemap) {
        glDeleteTextures(1, &resources_.prefilter_cubemap);
        resources_.prefilter_cubemap = 0;
    }
    if (resources_.irradiance_cubemap) {
        glDeleteTextures(1, &resources_.irradiance_cubemap);
        resources_.irradiance_cubemap = 0;
    }
    if (resources_.environment_cubemap) {
        glDeleteTextures(1, &resources_.environment_cubemap);
        resources_.environment_cubemap = 0;
    }
    if (resources_.hdr_texture) {
        glDeleteTextures(1, &resources_.hdr_texture);
        resources_.hdr_texture = 0;
    }
    if (resources_.capture_rbo) {
        glDeleteRenderbuffers(1, &resources_.capture_rbo);
        resources_.capture_rbo = 0;
    }
    if (resources_.capture_fbo) {
        glDeleteFramebuffers(1, &resources_.capture_fbo);
        resources_.capture_fbo = 0;
    }

    resources_.prefilter_mip_levels = 0;
    ibl_ready_ = false;
}

void PBRState::EnsureGBuffer(int width, int height)
{
    if (resources_.g_buffer_fbo && resources_.g_buffer_width == width &&
        resources_.g_buffer_height == height) {
        return;
    }

    DestroyGBuffer();
    CreateGBuffer(width, height);
}

void PBRState::SyncCamera(float width, float height)
{
    camera_.camera.SetPosition(camera_.position);
    camera_.camera.SetRotation(camera_.pitch, camera_.yaw);
    camera_.camera.SetPerspective(camera_.fov, width / height, 0.01f, 20.0f);
}

void PBRState::RenderGeometryPass(int width,
                                  int height,
                                  const glm::mat4& view,
                                  const glm::mat4& projection)
{
    static constexpr GLfloat clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, resources_.g_buffer_fbo);
    glViewport(0, 0, width, height);
    glClearBufferfv(GL_COLOR, 0, clear_color);
    glClearBufferfv(GL_COLOR, 1, clear_color);
    glClearBufferfv(GL_COLOR, 2, clear_color);
    glClearBufferfv(GL_COLOR, 3, clear_color);
    glClearBufferfv(GL_COLOR, 4, clear_color);
    glClearBufferfv(GL_COLOR, 5, clear_color);
    glClearBufferfv(GL_COLOR, 6, clear_color);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (!resources_.model || !resources_.model->IsLoaded()) {
        return;
    }

    resources_.geometry_shader->Bind();
    glUniformMatrix4fv(
        resources_.geometry_shader->Uniform("u_View"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(resources_.geometry_shader->Uniform("u_Projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(projection));

    glm::mat4 model_transform(1.0f);
    model_transform = glm::scale(model_transform, glm::vec3(material_.model_scale));
    resources_.model->Draw(*resources_.geometry_shader, model_transform);
    resources_.geometry_shader->Unbind();
}

void PBRState::RenderLightingPass(const std::array<GLint, 4>& viewport,
                                  GLint target_framebuffer,
                                  const glm::mat4& view,
                                  const glm::mat4& projection)
{
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(target_framebuffer));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_DEPTH_TEST);
    glClearColor(lighting_.background_color.r,
                 lighting_.background_color.g,
                 lighting_.background_color.b,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 inverse_view = glm::inverse(view);
    const glm::mat4 inverse_projection = glm::inverse(projection);

    resources_.lighting_shader->Bind();
    glUniform3fv(
        resources_.lighting_shader->Uniform("u_ViewPos"), 1, glm::value_ptr(camera_.position));
    glUniform3fv(resources_.lighting_shader->Uniform("u_Light.position"),
                 1,
                 glm::value_ptr(lighting_.light.position));
    glUniform3fv(resources_.lighting_shader->Uniform("u_Light.color"),
                 1,
                 glm::value_ptr(lighting_.light.color));
    glUniform1f(resources_.lighting_shader->Uniform("u_Light.intensity"),
                lighting_.light.intensity);
    glUniform1f(resources_.lighting_shader->Uniform("u_AmbientIntensity"),
                lighting_.ambient_intensity);
    glUniform1f(resources_.lighting_shader->Uniform("u_IblIntensity"), lighting_.ibl_intensity);
    glUniform1f(resources_.lighting_shader->Uniform("u_EnvironmentIntensity"),
                lighting_.environment_intensity);
    glUniform1f(resources_.lighting_shader->Uniform("u_BackgroundLod"), lighting_.background_lod);
    glUniform1f(resources_.lighting_shader->Uniform("u_PrefilterMaxLod"),
                static_cast<float>(std::max(resources_.prefilter_mip_levels - 1, 0)));
    glUniform1f(resources_.lighting_shader->Uniform("u_Exposure"), lighting_.exposure);
    glUniform1i(resources_.lighting_shader->Uniform("u_ShowEnvironment"),
                lighting_.show_environment ? 1 : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_HasIBL"), ibl_ready_ ? 1 : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_DebugOutputMode"),
                debug_.lighting_output_mode);
    glUniform3fv(resources_.lighting_shader->Uniform("u_BackgroundColor"),
                 1,
                 glm::value_ptr(lighting_.background_color));
    glUniformMatrix4fv(resources_.lighting_shader->Uniform("u_InverseView"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(inverse_view));
    glUniformMatrix4fv(resources_.lighting_shader->Uniform("u_InverseProjection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(inverse_projection));

    glBindTextureUnit(0, resources_.g_position_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GPosition"), 0);
    glBindTextureUnit(1, resources_.g_normal_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GNormal"), 1);
    glBindTextureUnit(2, resources_.g_albedo_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GAlbedo"), 2);
    glBindTextureUnit(3, resources_.g_metallic_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GMetallic"), 3);
    glBindTextureUnit(4, resources_.g_roughness_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GRoughness"), 4);
    glBindTextureUnit(5, resources_.g_ao_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GAO"), 5);
    glBindTextureUnit(6, resources_.g_emissive_texture);
    glUniform1i(resources_.lighting_shader->Uniform("u_GEmissive"), 6);

    glBindTextureUnit(7, ibl_ready_ ? resources_.environment_cubemap : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_EnvironmentMap"), 7);
    glBindTextureUnit(8, ibl_ready_ ? resources_.irradiance_cubemap : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_IrradianceMap"), 8);
    glBindTextureUnit(9, ibl_ready_ ? resources_.prefilter_cubemap : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_PrefilterMap"), 9);
    glBindTextureUnit(10, ibl_ready_ ? resources_.brdf_lut_texture : 0);
    glUniform1i(resources_.lighting_shader->Uniform("u_BrdfLut"), 10);

    RenderQuad();
    resources_.lighting_shader->Unbind();
}

void PBRState::RenderQuad()
{
    if (!resources_.quad_vao) {
        return;
    }

    resources_.quad_vao->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    resources_.quad_vao->Unbind();
}

void PBRState::RenderCube()
{
    if (!resources_.cube_vao) {
        return;
    }

    resources_.cube_vao->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    resources_.cube_vao->Unbind();
}

bool PBRState::OnKeyPressed(int keycode)
{
    input_.keys_down.insert(keycode);
    return false;
}

bool PBRState::OnKeyReleased(int keycode)
{
    input_.keys_down.erase(keycode);
    return false;
}

bool PBRState::OnMouseButtonPressed(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        input_.right_mouse_down = true;
        input_.first_mouse = true;
        return true;
    }

    return false;
}

bool PBRState::OnMouseButtonReleased(int button)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        input_.right_mouse_down = false;
        return true;
    }

    return false;
}

bool PBRState::OnMouseMoved(float x, float y)
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

bool PBRState::OnMouseScrolled(float /*x_offset*/, float y_offset)
{
    camera_.fov -= y_offset * 2.0f;
    camera_.fov = glm::clamp(camera_.fov, 10.0f, 120.0f);
    return true;
}
