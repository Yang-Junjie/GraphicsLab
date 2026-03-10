#include "Framebuffer.hpp"

namespace gl {

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Create(int width, int height)
{
    width_ = width;
    height_ = height;

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &color_);
    glBindTexture(GL_TEXTURE_2D, color_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height)
{
    if (width == width_ && height == height_) {
        return;
    }
    Destroy();
    Create(width, height);
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
