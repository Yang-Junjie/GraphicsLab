#include "Buffer.hpp"

namespace gl {

Buffer::Buffer()
{
    glGenBuffers(1, &id_);
}

Buffer::~Buffer()
{
    if (id_) {
        glDeleteBuffers(1, &id_);
    }
}

void Buffer::Bind(GLenum target) const
{
    glBindBuffer(target, id_);
}

void Buffer::Unbind(GLenum target) const
{
    glBindBuffer(target, 0);
}

GLuint Buffer::Id() const
{
    return id_;
}

void Buffer::Upload(GLenum target, GLsizeiptr size, const void* data, GLenum usage) const
{
    glBindBuffer(target, id_);
    glBufferData(target, size, data, usage);
}

void Buffer::BindBase(GLenum target, GLuint index) const
{
    glBindBufferBase(target, index, id_);
}

} // namespace gl
