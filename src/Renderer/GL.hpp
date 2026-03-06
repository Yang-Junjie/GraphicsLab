#pragma once

#include <glad/glad.h>

namespace gl {

// RAII Shader Program
class Shader {
public:
    Shader() = default;
    ~Shader() { if (id_) glDeleteProgram(id_); }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept : id_(o.id_) { o.id_ = 0; }
    Shader& operator=(Shader&& o) noexcept {
        if (this != &o) { if (id_) glDeleteProgram(id_); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    bool Compile(const char* vert_src, const char* frag_src) {
        GLuint vs = CompileStage(GL_VERTEX_SHADER, vert_src);
        GLuint fs = CompileStage(GL_FRAGMENT_SHADER, frag_src);
        if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return false; }

        id_ = glCreateProgram();
        glAttachShader(id_, vs);
        glAttachShader(id_, fs);
        glLinkProgram(id_);
        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint ok;
        glGetProgramiv(id_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
            glDeleteProgram(id_);
            id_ = 0;
            return false;
        }
        return true;
    }

    void Bind() const { glUseProgram(id_); }
    void Unbind() const { glUseProgram(0); }
    GLuint Id() const { return id_; }
    GLint Uniform(const char* name) const { return glGetUniformLocation(id_, name); }

private:
    GLuint id_ = 0;

    static GLuint CompileStage(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
};

// RAII Vertex Array Object
class VertexArray {
public:
    VertexArray() { glGenVertexArrays(1, &id_); }
    ~VertexArray() { if (id_) glDeleteVertexArrays(1, &id_); }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const { glBindVertexArray(id_); }
    void Unbind() const { glBindVertexArray(0); }
    GLuint Id() const { return id_; }

private:
    GLuint id_ = 0;
};

// RAII Buffer Object (VBO / EBO / SSBO)
class Buffer {
public:
    Buffer() { glGenBuffers(1, &id_); }
    ~Buffer() { if (id_) glDeleteBuffers(1, &id_); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void Bind(GLenum target) const { glBindBuffer(target, id_); }
    void Unbind(GLenum target) const { glBindBuffer(target, 0); }
    GLuint Id() const { return id_; }

    void Upload(GLenum target, GLsizeiptr size, const void* data, GLenum usage) const {
        glBindBuffer(target, id_);
        glBufferData(target, size, data, usage);
    }

    void BindBase(GLenum target, GLuint index) const {
        glBindBufferBase(target, index, id_);
    }

private:
    GLuint id_ = 0;
};

// RAII Framebuffer Object
class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer() { Destroy(); }

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Create(int width, int height) {
        width_ = width;
        height_ = height;

        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        glGenTextures(1, &color_);
        glBindTexture(GL_TEXTURE_2D, color_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Resize(int width, int height) {
        if (width == width_ && height == height_) return;
        Destroy();
        Create(width, height);
    }

    void Bind() {
        glGetIntegerv(GL_VIEWPORT, prev_viewport_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, width_, height_);
    }

    void Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(prev_viewport_[0], prev_viewport_[1],
                   prev_viewport_[2], prev_viewport_[3]);
    }

    GLuint ColorTexture() const { return color_; }
    int Width() const { return width_; }
    int Height() const { return height_; }

private:
    void Destroy() {
        if (color_) { glDeleteTextures(1, &color_); color_ = 0; }
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
        width_ = height_ = 0;
    }

    GLuint fbo_ = 0;
    GLuint color_ = 0;
    int width_ = 0, height_ = 0;
    GLint prev_viewport_[4] = {};
};

} // namespace gl
