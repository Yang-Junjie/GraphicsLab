#include "TextureScene.hpp"


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

TextureScene::TextureScene()
    : Scene("Texture Quad")
{}

TextureScene::~TextureScene()
{
    for (auto& img : images_) {
        if (img.texture) {
            glDeleteTextures(1, &img.texture);
        }
    }
}

void TextureScene::OnEnter()
{
    InitResources();
}

void TextureScene::OnExit() {}

GLuint TextureScene::LoadTexture(const std::string& path, int& w, int& h)
{
    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!data) return 0;

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

void TextureScene::LoadImages()
{
    const char* files[] = {"res/image/container.jpg", "res/image/awesomeface.png"};
    const char* names[] = {"container.jpg", "awesomeface.png"};

    for (int i = 0; i < 2; ++i) {
        ImageInfo info;
        info.name = names[i];
        info.path = files[i];
        info.texture = LoadTexture(info.path, info.width, info.height);
        images_.push_back(info);
    }
}

void TextureScene::InitResources()
{
    if (initialized_) return;

    // Quad vertices: position (x,y) + texcoord (u,v)
    // Origin at center, half-extent 0.5
    float verts[] = {
        // pos        // uv
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
    };
    uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    vao_.Bind();
    vbo_.Upload(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    // texcoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
                          
    ebo_.Upload(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    vao_.Unbind();

    shader_.CompileFromFile("shaders/textured_quad.vert", "shaders/textured_quad.frag");
    proj_loc_ = shader_.Uniform("u_Proj");
    model_loc_ = shader_.Uniform("u_Model");
    tex_loc_ = shader_.Uniform("u_Texture");

    LoadImages();

    initialized_ = true;
}

void TextureScene::OnRender(float width, float height)
{
    if (images_.empty() || !images_[current_image_].texture) return;

    const auto& img = images_[current_image_];

    glm::mat4 proj = glm::ortho(0.0f, width, height, 0.0f);

    // Scale quad to image aspect ratio, fit within viewport
    float img_aspect = static_cast<float>(img.width) / static_cast<float>(img.height);
    float vp_aspect = width / height;

    float quad_w, quad_h;
    if (img_aspect > vp_aspect) {
        quad_w = width * 0.8f;
        quad_h = quad_w / img_aspect;
    } else {
        quad_h = height * 0.8f;
        quad_w = quad_h * img_aspect;
    }

    quad_w *= scale_;
    quad_h *= scale_;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(quad_w, quad_h, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    shader_.Bind();
    glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(model_loc_, 1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img.texture);
    glUniform1i(tex_loc_, 0);

    vao_.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextureScene::OnRenderUI()
{
    if (images_.empty()) {
        ImGui::Text("No images loaded");
        return;
    }

    const char* current_name = images_[current_image_].name.c_str();
    if (ImGui::BeginCombo("Image", current_name)) {
        for (int i = 0; i < static_cast<int>(images_.size()); ++i) {
            bool selected = (i == current_image_);
            if (ImGui::Selectable(images_[i].name.c_str(), selected)) {
                current_image_ = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SliderFloat("Scale", &scale_, 0.1f, 3.0f);

    const auto& img = images_[current_image_];
    ImGui::Separator();
    ImGui::Text("Size: %d x %d", img.width, img.height);
}
