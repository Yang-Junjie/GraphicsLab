#pragma once

#include <glad/glad.h>

namespace gl {

// RAII Framebuffer Object
class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Create(int width, int height);
    void Resize(int width, int height);
    void Bind();
    void Unbind();

    GLuint ColorTexture() const;
    int Width() const;
    int Height() const;

private:
    void Destroy();

    GLuint fbo_ = 0;
    GLuint color_ = 0;
    GLuint depth_rbo_ = 0;
    int width_ = 0, height_ = 0;
    GLint prev_viewport_[4] = {};
};

} // namespace gl
