#pragma once

#include "Backend/OpenGL/Shader.hpp"

#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Material {
public:
    Material() = default;
    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(Material&& other) noexcept;
    Material& operator=(Material&& other) noexcept;

    void SetBaseColorFactor(const glm::vec4& color);

    bool LoadBaseColorTextureFromFile(const std::filesystem::path& path);
    bool LoadBaseColorTextureFromMemory(const unsigned char* bytes, int size_bytes);

    void Bind(const gl::Shader& shader, int texture_slot = 0) const;

private:
    void ReleaseTexture();
    bool UploadTextureFromPixels(unsigned char* pixels, int width, int height, int channels);

    glm::vec4 base_color_factor_ = glm::vec4(1.0f);
    GLuint base_color_texture_ = 0;
    bool has_base_color_texture_ = false;
};
