#include "Buffer.hpp"

namespace gl {

Buffer::Buffer()
{
    glCreateBuffers(1, &id_);
}

Buffer::~Buffer()
{
    if (id_) {
        glDeleteBuffers(1, &id_);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
{
    id_ = other.id_;
    other.id_ = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        glDeleteBuffers(1, &id_);
        id_ = other.id_;
        other.id_ = 0;
    }
    return *this;
}

GLuint Buffer::Id() const
{
    return id_;
}

void Buffer::Storage(GLsizeiptr size, const void* data, GLenum usage) const
{
    glNamedBufferData(id_, size, data, usage);
}

void Buffer::SubData(GLintptr offset, GLsizeiptr size, const void* data) const
{
    glNamedBufferSubData(id_, offset, size, data);
}

void Buffer::BindBase(GLenum target, GLuint index) const
{
    glBindBufferBase(target, index, id_);
}

} // namespace gl
