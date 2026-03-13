#include "Material.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <string>

Material::~Material()
{
    ReleaseTexture();
}

Material::Material(Material&& other) noexcept
    : base_color_factor_(other.base_color_factor_)
    , base_color_texture_(other.base_color_texture_)
    , has_base_color_texture_(other.has_base_color_texture_)
{
    other.base_color_texture_ = 0;
    other.has_base_color_texture_ = false;
}

Material& Material::operator=(Material&& other) noexcept
{
    if (this != &other) {
        ReleaseTexture();

        base_color_factor_ = other.base_color_factor_;
        base_color_texture_ = other.base_color_texture_;
        has_base_color_texture_ = other.has_base_color_texture_;

        other.base_color_texture_ = 0;
        other.has_base_color_texture_ = false;
    }

    return *this;
}

void Material::SetBaseColorFactor(const glm::vec4& color)
{
    base_color_factor_ = color;
}

bool Material::LoadBaseColorTextureFromFile(const std::filesystem::path& path)
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

    const bool success = UploadTextureFromPixels(pixels, width, height, channels);
    stbi_image_free(pixels);
    return success;
}

bool Material::LoadBaseColorTextureFromMemory(const unsigned char* bytes, int size_bytes)
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

    const bool success = UploadTextureFromPixels(pixels, width, height, channels);
    stbi_image_free(pixels);
    return success;
}

void Material::Bind(const gl::Shader& shader, int texture_slot) const
{
    glUniform4fv(shader.Uniform("u_BaseColorFactor"),
                 1,
                 glm::value_ptr(base_color_factor_));

    glUniform1i(shader.Uniform("u_UseBaseColorTexture"), has_base_color_texture_ ? 1 : 0);

    if (has_base_color_texture_) {
        glActiveTexture(GL_TEXTURE0 + texture_slot);
        glBindTexture(GL_TEXTURE_2D, base_color_texture_);
    }

    glUniform1i(shader.Uniform("u_BaseColorTexture"), texture_slot);
}

void Material::ReleaseTexture()
{
    if (base_color_texture_) {
        glDeleteTextures(1, &base_color_texture_);
        base_color_texture_ = 0;
    }
    has_base_color_texture_ = false;
}

bool Material::UploadTextureFromPixels(unsigned char* pixels,
                                       int width,
                                       int height,
                                       int channels)
{
    if (!pixels || width <= 0 || height <= 0) {
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    ReleaseTexture();

    glGenTextures(1, &base_color_texture_);
    glBindTexture(GL_TEXTURE_2D, base_color_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 format,
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    has_base_color_texture_ = true;
    return true;
}

