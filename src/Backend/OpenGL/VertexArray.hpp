#pragma once

#include <glad/glad.h>

namespace gl {

// RAII Vertex Array Object
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const;
    void Unbind() const;
    GLuint Id() const;

private:
    GLuint id_ = 0;
};

} // namespace gl
