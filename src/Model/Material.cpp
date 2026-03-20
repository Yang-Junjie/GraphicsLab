#include "Material.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <string>

Material::~Material()
{
    ReleaseTextures();
}

Material::Material(Material&& other) noexcept
    : base_color_factor_(other.base_color_factor_)
    , metallic_factor_(other.metallic_factor_)
    , roughness_factor_(other.roughness_factor_)
    , normal_scale_(other.normal_scale_)
    , occlusion_strength_(other.occlusion_strength_)
    , emissive_factor_(other.emissive_factor_)
    , base_color_texture_(other.base_color_texture_)
    , metallic_roughness_texture_(other.metallic_roughness_texture_)
    , normal_texture_(other.normal_texture_)
    , occlusion_texture_(other.occlusion_texture_)
    , emissive_texture_(other.emissive_texture_)
    , has_base_color_texture_(other.has_base_color_texture_)
    , has_metallic_roughness_texture_(other.has_metallic_roughness_texture_)
    , has_normal_texture_(other.has_normal_texture_)
    , has_occlusion_texture_(other.has_occlusion_texture_)
    , has_emissive_texture_(other.has_emissive_texture_)
{
    other.base_color_texture_ = 0;
    other.metallic_roughness_texture_ = 0;
    other.normal_texture_ = 0;
    other.occlusion_texture_ = 0;
    other.emissive_texture_ = 0;
    other.has_base_color_texture_ = false;
    other.has_metallic_roughness_texture_ = false;
    other.has_normal_texture_ = false;
    other.has_occlusion_texture_ = false;
    other.has_emissive_texture_ = false;
}

Material& Material::operator=(Material&& other) noexcept
{
    if (this != &other) {
        ReleaseTextures();

        base_color_factor_ = other.base_color_factor_;
        metallic_factor_ = other.metallic_factor_;
        roughness_factor_ = other.roughness_factor_;
        normal_scale_ = other.normal_scale_;
        occlusion_strength_ = other.occlusion_strength_;
        emissive_factor_ = other.emissive_factor_;

        base_color_texture_ = other.base_color_texture_;
        metallic_roughness_texture_ = other.metallic_roughness_texture_;
        normal_texture_ = other.normal_texture_;
        occlusion_texture_ = other.occlusion_texture_;
        emissive_texture_ = other.emissive_texture_;

        has_base_color_texture_ = other.has_base_color_texture_;
        has_metallic_roughness_texture_ = other.has_metallic_roughness_texture_;
        has_normal_texture_ = other.has_normal_texture_;
        has_occlusion_texture_ = other.has_occlusion_texture_;
        has_emissive_texture_ = other.has_emissive_texture_;

        other.base_color_texture_ = 0;
        other.metallic_roughness_texture_ = 0;
        other.normal_texture_ = 0;
        other.occlusion_texture_ = 0;
        other.emissive_texture_ = 0;
        other.has_base_color_texture_ = false;
        other.has_metallic_roughness_texture_ = false;
        other.has_normal_texture_ = false;
        other.has_occlusion_texture_ = false;
        other.has_emissive_texture_ = false;
    }

    return *this;
}

void Material::SetBaseColorFactor(const glm::vec4& color)
{
    base_color_factor_ = color;
}

void Material::SetMetallicFactor(float factor)
{
    metallic_factor_ = factor;
}

void Material::SetRoughnessFactor(float factor)
{
    roughness_factor_ = factor;
}

void Material::SetNormalScale(float scale)
{
    normal_scale_ = scale;
}

void Material::SetOcclusionStrength(float strength)
{
    occlusion_strength_ = strength;
}

void Material::SetEmissiveFactor(const glm::vec3& factor)
{
    emissive_factor_ = factor;
}

bool Material::LoadBaseColorTextureFromFile(const std::filesystem::path& path)
{
    return LoadTextureFromFile(base_color_texture_, has_base_color_texture_, path, true);
}

bool Material::LoadBaseColorTextureFromMemory(const unsigned char* bytes, int size_bytes)
{
    return LoadTextureFromMemory(base_color_texture_,
                                 has_base_color_texture_,
                                 bytes,
                                 size_bytes,
                                 true);
}

bool Material::LoadMetallicRoughnessTextureFromFile(const std::filesystem::path& path)
{
    return LoadTextureFromFile(metallic_roughness_texture_,
                               has_metallic_roughness_texture_,
                               path,
                               false);
}

bool Material::LoadMetallicRoughnessTextureFromMemory(const unsigned char* bytes, int size_bytes)
{
    return LoadTextureFromMemory(metallic_roughness_texture_,
                                 has_metallic_roughness_texture_,
                                 bytes,
                                 size_bytes,
                                 false);
}

bool Material::LoadNormalTextureFromFile(const std::filesystem::path& path)
{
    return LoadTextureFromFile(normal_texture_, has_normal_texture_, path, false);
}

bool Material::LoadNormalTextureFromMemory(const unsigned char* bytes, int size_bytes)
{
    return LoadTextureFromMemory(normal_texture_, has_normal_texture_, bytes, size_bytes, false);
}

bool Material::LoadOcclusionTextureFromFile(const std::filesystem::path& path)
{
    return LoadTextureFromFile(occlusion_texture_, has_occlusion_texture_, path, false);
}

bool Material::LoadOcclusionTextureFromMemory(const unsigned char* bytes, int size_bytes)
{
    return LoadTextureFromMemory(
        occlusion_texture_, has_occlusion_texture_, bytes, size_bytes, false);
}

bool Material::LoadEmissiveTextureFromFile(const std::filesystem::path& path)
{
    return LoadTextureFromFile(emissive_texture_, has_emissive_texture_, path, true);
}

bool Material::LoadEmissiveTextureFromMemory(const unsigned char* bytes, int size_bytes)
{
    return LoadTextureFromMemory(
        emissive_texture_, has_emissive_texture_, bytes, size_bytes, true);
}

void Material::Bind(const gl::Shader& shader, int texture_slot) const
{
    const GLint base_color_slot = texture_slot;
    const GLint metallic_roughness_slot = texture_slot + 1;
    const GLint normal_slot = texture_slot + 2;
    const GLint occlusion_slot = texture_slot + 3;
    const GLint emissive_slot = texture_slot + 4;

    glUniform4fv(shader.Uniform("u_BaseColorFactor"), 1, glm::value_ptr(base_color_factor_));
    glUniform1f(shader.Uniform("u_MetallicFactor"), metallic_factor_);
    glUniform1f(shader.Uniform("u_RoughnessFactor"), roughness_factor_);
    glUniform1f(shader.Uniform("u_NormalScale"), normal_scale_);
    glUniform1f(shader.Uniform("u_OcclusionStrength"), occlusion_strength_);
    glUniform3fv(shader.Uniform("u_EmissiveFactor"), 1, glm::value_ptr(emissive_factor_));

    glUniform1i(shader.Uniform("u_UseBaseColorTexture"), has_base_color_texture_ ? 1 : 0);
    glUniform1i(shader.Uniform("u_UseMetallicRoughnessTexture"),
                has_metallic_roughness_texture_ ? 1 : 0);
    glUniform1i(shader.Uniform("u_UseNormalTexture"), has_normal_texture_ ? 1 : 0);
    glUniform1i(shader.Uniform("u_UseOcclusionTexture"), has_occlusion_texture_ ? 1 : 0);
    glUniform1i(shader.Uniform("u_UseEmissiveTexture"), has_emissive_texture_ ? 1 : 0);

    glBindTextureUnit(base_color_slot, has_base_color_texture_ ? base_color_texture_ : 0);
    glBindTextureUnit(metallic_roughness_slot,
                      has_metallic_roughness_texture_ ? metallic_roughness_texture_ : 0);
    glBindTextureUnit(normal_slot, has_normal_texture_ ? normal_texture_ : 0);
    glBindTextureUnit(occlusion_slot, has_occlusion_texture_ ? occlusion_texture_ : 0);
    glBindTextureUnit(emissive_slot, has_emissive_texture_ ? emissive_texture_ : 0);

    glUniform1i(shader.Uniform("u_BaseColorTexture"), base_color_slot);
    glUniform1i(shader.Uniform("u_MetallicRoughnessTexture"), metallic_roughness_slot);
    glUniform1i(shader.Uniform("u_NormalTexture"), normal_slot);
    glUniform1i(shader.Uniform("u_OcclusionTexture"), occlusion_slot);
    glUniform1i(shader.Uniform("u_EmissiveTexture"), emissive_slot);
}

bool Material::LoadTextureFromFile(GLuint& texture,
                                   bool& has_texture,
                                   const std::filesystem::path& path,
                                   bool srgb)
{
    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;

    const std::string path_string = path.string();
    unsigned char* pixels = stbi_load(path_string.c_str(), &width, &height, &channels, 0);
    if (!pixels) {
        return false;
    }

    const bool success = UploadTextureFromPixels(texture, has_texture, pixels, width, height, channels, srgb);
    stbi_image_free(pixels);
    return success;
}

bool Material::LoadTextureFromMemory(GLuint& texture,
                                     bool& has_texture,
                                     const unsigned char* bytes,
                                     int size_bytes,
                                     bool srgb)
{
    if (!bytes || size_bytes <= 0) {
        return false;
    }

    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels =
        stbi_load_from_memory(bytes, size_bytes, &width, &height, &channels, 0);
    if (!pixels) {
        return false;
    }

    const bool success = UploadTextureFromPixels(texture, has_texture, pixels, width, height, channels, srgb);
    stbi_image_free(pixels);
    return success;
}

void Material::ReleaseTexture(GLuint& texture, bool& has_texture)
{
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
    has_texture = false;
}

void Material::ReleaseTextures()
{
    ReleaseTexture(base_color_texture_, has_base_color_texture_);
    ReleaseTexture(metallic_roughness_texture_, has_metallic_roughness_texture_);
    ReleaseTexture(normal_texture_, has_normal_texture_);
    ReleaseTexture(occlusion_texture_, has_occlusion_texture_);
    ReleaseTexture(emissive_texture_, has_emissive_texture_);
}

bool Material::UploadTextureFromPixels(GLuint& texture,
                                       bool& has_texture,
                                       unsigned char* pixels,
                                       int width,
                                       int height,
                                       int channels,
                                       bool srgb)
{
    if (!pixels || width <= 0 || height <= 0) {
        return false;
    }

    GLenum format = GL_RGB;
    GLenum internal_format = srgb ? GL_SRGB8 : GL_RGB8;
    if (channels == 1) {
        format = GL_RED;
        internal_format = GL_R8;
    } else if (channels == 2) {
        format = GL_RG;
        internal_format = GL_RG8;
    } else if (channels == 4) {
        format = GL_RGBA;
        internal_format = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    }

    ReleaseTexture(texture, has_texture);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal_format,
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    has_texture = true;
    return true;
}
