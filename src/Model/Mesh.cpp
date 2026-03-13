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

    vao_->Bind();

    vbo_->Upload(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices_.size() * sizeof(MeshVertex)),
                 vertices_.data(),
                 GL_STATIC_DRAW);
    ebo_->Upload(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices_.size() * sizeof(std::uint32_t)),
                 indices_.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, texcoord)));

    vao_->Unbind();
}
