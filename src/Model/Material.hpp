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
    void SetMetallicFactor(float factor);
    void SetRoughnessFactor(float factor);
    void SetNormalScale(float scale);
    void SetOcclusionStrength(float strength);
    void SetEmissiveFactor(const glm::vec3& factor);

    bool LoadBaseColorTextureFromFile(const std::filesystem::path& path);
    bool LoadBaseColorTextureFromMemory(const unsigned char* bytes, int size_bytes);
    bool LoadMetallicRoughnessTextureFromFile(const std::filesystem::path& path);
    bool LoadMetallicRoughnessTextureFromMemory(const unsigned char* bytes, int size_bytes);
    bool LoadNormalTextureFromFile(const std::filesystem::path& path);
    bool LoadNormalTextureFromMemory(const unsigned char* bytes, int size_bytes);
    bool LoadOcclusionTextureFromFile(const std::filesystem::path& path);
    bool LoadOcclusionTextureFromMemory(const unsigned char* bytes, int size_bytes);
    bool LoadEmissiveTextureFromFile(const std::filesystem::path& path);
    bool LoadEmissiveTextureFromMemory(const unsigned char* bytes, int size_bytes);

    void Bind(const gl::Shader& shader, int texture_slot = 0) const;

private:
    bool LoadTextureFromFile(GLuint& texture,
                             bool& has_texture,
                             const std::filesystem::path& path,
                             bool srgb);
    bool LoadTextureFromMemory(GLuint& texture,
                               bool& has_texture,
                               const unsigned char* bytes,
                               int size_bytes,
                               bool srgb);
    void ReleaseTexture(GLuint& texture, bool& has_texture);
    void ReleaseTextures();
    bool UploadTextureFromPixels(GLuint& texture,
                                 bool& has_texture,
                                 unsigned char* pixels,
                                 int width,
                                 int height,
                                 int channels,
                                 bool srgb);

    glm::vec4 base_color_factor_ = glm::vec4(1.0f);
    float metallic_factor_ = 1.0f;
    float roughness_factor_ = 1.0f;
    float normal_scale_ = 1.0f;
    float occlusion_strength_ = 1.0f;
    glm::vec3 emissive_factor_ = glm::vec3(0.0f);

    GLuint base_color_texture_ = 0;
    GLuint metallic_roughness_texture_ = 0;
    GLuint normal_texture_ = 0;
    GLuint occlusion_texture_ = 0;
    GLuint emissive_texture_ = 0;

    bool has_base_color_texture_ = false;
    bool has_metallic_roughness_texture_ = false;
    bool has_normal_texture_ = false;
    bool has_occlusion_texture_ = false;
    bool has_emissive_texture_ = false;
};
