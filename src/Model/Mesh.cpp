#include "Mesh.hpp"

#include <cstddef>
#include <cstdint>

Mesh::Mesh(std::vector<MeshVertex> vertices,
           std::vector<std::uint32_t> indices,
           std::vector<SubMesh> submeshes)
    : vertices_(std::move(vertices)),
      indices_(std::move(indices)),
      submeshes_(std::move(submeshes))
{
    SetupGpuResources();
}

void Mesh::Draw(const std::vector<Material>& materials, const gl::Shader& shader) const
{
    if (!vao_ || submeshes_.empty()) {
        return;
    }

    vao_->Bind();

    for (const auto& submesh : submeshes_) {
        if (submesh.material_index < materials.size()) {
            materials[submesh.material_index].Bind(shader, 0);
        }

        glDrawElements(
            submesh.primitive_mode,
            static_cast<GLsizei>(submesh.index_count),
            GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(submesh.index_offset) *
                                          sizeof(std::uint32_t)));
    }

    vao_->Unbind();
}

std::size_t Mesh::GetSubMeshCount() const
{
    return submeshes_.size();
}

void Mesh::SetupGpuResources()
{
    if (vertices_.empty() || indices_.empty()) {
        return;
    }

    vao_ = std::make_unique<gl::VertexArray>();
    vbo_ = std::make_unique<gl::Buffer>();
    ebo_ = std::make_unique<gl::Buffer>();

    vbo_->Storage(static_cast<GLsizeiptr>(vertices_.size() * sizeof(MeshVertex)),
                  vertices_.data(),
                  GL_STATIC_DRAW);
    ebo_->Storage(static_cast<GLsizeiptr>(indices_.size() * sizeof(std::uint32_t)),
                  indices_.data(),
                  GL_STATIC_DRAW);

    GLuint vao = vao_->Id();
    glVertexArrayVertexBuffer(vao, 0, vbo_->Id(), 0, sizeof(MeshVertex));
    glVertexArrayElementBuffer(vao, ebo_->Id());

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, position));
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, normal));
    glVertexArrayAttribBinding(vao, 1, 0);

    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, texcoord));
    glVertexArrayAttribBinding(vao, 2, 0);
}
