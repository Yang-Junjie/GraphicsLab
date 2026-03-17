#include "Framebuffer.hpp"

namespace gl {

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Create(int width, int height, GLuint internal_format)
{
    width_ = width;
    height_ = height;
    internal_format_ = internal_format;

    // Choose pixel type: float for HDR formats, unsigned byte otherwise
    GLenum pixel_type = GL_UNSIGNED_BYTE;
    if (internal_format == GL_RGBA16F || internal_format == GL_RGBA32F
        || internal_format == GL_RGB16F || internal_format == GL_RGB32F) {
        pixel_type = GL_FLOAT;
    }

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &color_);
    glBindTexture(GL_TEXTURE_2D, color_);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, pixel_type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);

    glGenRenderbuffers(1, &depth_rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height)
{
    if (width == width_ && height == height_) {
        return;
    }
    Destroy();
    Create(width, height, internal_format_);
}

void Framebuffer::Bind()
{
    glGetIntegerv(GL_VIEWPORT, prev_viewport_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prev_viewport_[0], prev_viewport_[1], prev_viewport_[2], prev_viewport_[3]);
}

GLuint Framebuffer::ColorTexture() const
{
    return color_;
}

int Framebuffer::Width() const
{
    return width_;
}

int Framebuffer::Height() const
{
    return height_;
}

void Framebuffer::Destroy()
{
    if (depth_rbo_) {
        glDeleteRenderbuffers(1, &depth_rbo_);
        depth_rbo_ = 0;
    }
    if (color_) {
        glDeleteTextures(1, &color_);
        color_ = 0;
    }
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    width_ = height_ = 0;
}

} // namespace gl
