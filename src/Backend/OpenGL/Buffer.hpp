#pragma once
#include <glad/glad.h>

namespace gl {

class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    GLuint Id() const;

    void Storage(GLsizeiptr size, const void* data, GLenum usage) const;
    void SubData(GLintptr offset, GLsizeiptr size, const void* data) const;

    void BindBase(GLenum target, GLuint index) const;

private:
    GLuint id_ = 0;
};

} // namespace gl
