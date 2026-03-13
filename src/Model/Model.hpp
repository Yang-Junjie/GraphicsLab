#pragma once

#include "Mesh.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

class Model {
public:
    Model() = default;

    bool LoadFromGLTF(const std::filesystem::path& path);
    void Draw(const gl::Shader& shader, const glm::mat4& root_transform = glm::mat4(1.0f)) const;

    bool IsLoaded() const;

    std::size_t GetMeshCount() const;
    std::size_t GetMaterialCount() const;
    std::size_t GetInstanceCount() const;

private:
    struct MeshInstance {
        std::size_t mesh_index = 0;
        glm::mat4 transform = glm::mat4(1.0f);
    };

    void Clear();

    std::vector<Mesh> meshes_;
    std::vector<Material> materials_;
    std::vector<MeshInstance> instances_;

    bool loaded_ = false;
};
