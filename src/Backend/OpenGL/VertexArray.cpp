#include "VertexArray.hpp"

namespace gl {

VertexArray::VertexArray()
{
    glCreateVertexArrays(1, &id_);
}

VertexArray::~VertexArray()
{
    if (id_) {
        glDeleteVertexArrays(1, &id_);
    }
}

void VertexArray::Bind() const
{
    glBindVertexArray(id_);
}

void VertexArray::Unbind() const
{
    glBindVertexArray(0);
}

GLuint VertexArray::Id() const
{
    return id_;
}

} // namespace gl
