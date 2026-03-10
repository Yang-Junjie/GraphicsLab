#pragma once

#include <glad/glad.h>

namespace gl {

// RAII Buffer Object (VBO / EBO / SSBO)
class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void Bind(GLenum target) const;
    void Unbind(GLenum target) const;
    GLuint Id() const;
    void Upload(GLenum target, GLsizeiptr size, const void* data, GLenum usage) const;
    void BindBase(GLenum target, GLuint index) const;

private:
    GLuint id_ = 0;
};

} // namespace gl
