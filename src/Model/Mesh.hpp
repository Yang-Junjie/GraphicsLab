#pragma once

#include "Backend/OpenGL/Buffer.hpp"
#include "Backend/OpenGL/VertexArray.hpp"
#include "Material.hpp"
#include "SubMesh.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct MeshVertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 texcoord = glm::vec2(0.0f);
    glm::vec4 tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
};

class Mesh {
public:
    Mesh() = default;
    Mesh(std::vector<MeshVertex> vertices,
         std::vector<std::uint32_t> indices,
         std::vector<SubMesh> submeshes);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void Draw(const std::vector<Material>& materials, const gl::Shader& shader) const;

    std::size_t GetSubMeshCount() const;

private:
    void SetupGpuResources();

    std::vector<MeshVertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<SubMesh> submeshes_;

    std::unique_ptr<gl::VertexArray> vao_;
    std::unique_ptr<gl::Buffer> vbo_;
    std::unique_ptr<gl::Buffer> ebo_;
};
