#include "Model.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <limits>
#include <optional>
#include <vector>

namespace {

glm::mat4 ToGlmMat4(const fastgltf::math::fmat4x4& matrix)
{
    glm::mat4 result(1.0f);
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result[col][row] = matrix[col][row];
        }
    }
    return result;
}

GLenum PrimitiveTypeToGL(fastgltf::PrimitiveType type)
{
    switch (type) {
        case fastgltf::PrimitiveType::Points:
            return GL_POINTS;
        case fastgltf::PrimitiveType::Lines:
            return GL_LINES;
        case fastgltf::PrimitiveType::LineLoop:
            return GL_LINE_LOOP;
        case fastgltf::PrimitiveType::LineStrip:
            return GL_LINE_STRIP;
        case fastgltf::PrimitiveType::Triangles:
            return GL_TRIANGLES;
        case fastgltf::PrimitiveType::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case fastgltf::PrimitiveType::TriangleFan:
            return GL_TRIANGLE_FAN;
        default:
            return GL_NONE;
    }
}

struct ByteSpan {
    const unsigned char* data = nullptr;
    std::size_t size = 0;
};

std::optional<ByteSpan> ReadBytesFromBuffer(const fastgltf::DataSource& source,
                                            const fastgltf::BufferView* view)
{
    return std::visit(
        fastgltf::visitor {
            [&](const fastgltf::sources::Array& array) -> std::optional<ByteSpan> {
                if (view != nullptr) {
                    if (view->byteOffset + view->byteLength > array.bytes.size()) {
                        return std::nullopt;
                    }
                    return ByteSpan {
                        reinterpret_cast<const unsigned char*>(array.bytes.data() +
                                                               view->byteOffset),
                        view->byteLength,
                    };
                }
                return ByteSpan {
                    reinterpret_cast<const unsigned char*>(array.bytes.data()),
                    array.bytes.size(),
                };
            },
            [&](const fastgltf::sources::Vector& vector) -> std::optional<ByteSpan> {
                if (view != nullptr) {
                    if (view->byteOffset + view->byteLength > vector.bytes.size()) {
                        return std::nullopt;
                    }
                    return ByteSpan {
                        reinterpret_cast<const unsigned char*>(vector.bytes.data() +
                                                               view->byteOffset),
                        view->byteLength,
                    };
                }
                return ByteSpan {
                    reinterpret_cast<const unsigned char*>(vector.bytes.data()),
                    vector.bytes.size(),
                };
            },
            [&](const fastgltf::sources::ByteView& bytes) -> std::optional<ByteSpan> {
                if (view != nullptr) {
                    if (view->byteOffset + view->byteLength > bytes.bytes.size()) {
                        return std::nullopt;
                    }
                    return ByteSpan {
                        reinterpret_cast<const unsigned char*>(bytes.bytes.data() +
                                                               view->byteOffset),
                        view->byteLength,
                    };
                }
                return ByteSpan {
                    reinterpret_cast<const unsigned char*>(bytes.bytes.data()),
                    bytes.bytes.size(),
                };
            },
            [&](const auto&) -> std::optional<ByteSpan> {
                return std::nullopt;
            },
        },
        source);
}

bool TryLoadMaterialTexture(Material& material,
                            const fastgltf::Asset& asset,
                            std::size_t texture_index,
                            const std::filesystem::path& model_directory)
{
    if (texture_index >= asset.textures.size()) {
        return false;
    }

    const auto& texture = asset.textures[texture_index];
    if (!texture.imageIndex.has_value()) {
        return false;
    }

    const std::size_t image_index = texture.imageIndex.value();
    if (image_index >= asset.images.size()) {
        return false;
    }

    const auto& image = asset.images[image_index];
    return std::visit(
        fastgltf::visitor {
            [&](const fastgltf::sources::URI& source) {
                if (!source.uri.isLocalPath()) {
                    return false;
                }

                const std::string uri_path(source.uri.path().begin(), source.uri.path().end());
                const std::filesystem::path image_path = model_directory / uri_path;
                return material.LoadBaseColorTextureFromFile(image_path);
            },
            [&](const fastgltf::sources::BufferView& source) {
                if (source.bufferViewIndex >= asset.bufferViews.size()) {
                    return false;
                }
                const auto& buffer_view = asset.bufferViews[source.bufferViewIndex];
                if (buffer_view.bufferIndex >= asset.buffers.size()) {
                    return false;
                }

                const auto bytes =
                    ReadBytesFromBuffer(asset.buffers[buffer_view.bufferIndex].data, &buffer_view);
                if (!bytes.has_value()) {
                    return false;
                }
                return material.LoadBaseColorTextureFromMemory(bytes->data,
                                                               static_cast<int>(bytes->size));
            },
            [&](const fastgltf::sources::Array& source) {
                return material.LoadBaseColorTextureFromMemory(
                    reinterpret_cast<const unsigned char*>(source.bytes.data()),
                    static_cast<int>(source.bytes.size()));
            },
            [&](const fastgltf::sources::Vector& source) {
                return material.LoadBaseColorTextureFromMemory(
                    reinterpret_cast<const unsigned char*>(source.bytes.data()),
                    static_cast<int>(source.bytes.size()));
            },
            [&](const fastgltf::sources::ByteView& source) {
                return material.LoadBaseColorTextureFromMemory(
                    reinterpret_cast<const unsigned char*>(source.bytes.data()),
                    static_cast<int>(source.bytes.size()));
            },
            [&](const auto&) {
                return false;
            },
        },
        image.data);
}

} // namespace

bool Model::LoadFromGLTF(const std::filesystem::path& path)
{
    Clear();

    if (!std::filesystem::exists(path)) {
        return false;
    }

    static constexpr auto kOptions = fastgltf::Options::DontRequireValidAssetMember |
                                     fastgltf::Options::AllowDouble |
                                     fastgltf::Options::LoadExternalBuffers |
                                     fastgltf::Options::LoadExternalImages |
                                     fastgltf::Options::GenerateMeshIndices;

    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform |
                            fastgltf::Extensions::KHR_mesh_quantization);

    auto file = fastgltf::MappedGltfFile::FromPath(path);
    if (!file) {
        return false;
    }

    auto asset_result = parser.loadGltf(file.get(), path.parent_path(), kOptions);
    if (asset_result.error() != fastgltf::Error::None) {
        return false;
    }

    fastgltf::Asset asset = std::move(asset_result.get());

    if (asset.materials.empty()) {
        materials_.emplace_back();
    } else {
        materials_.reserve(asset.materials.size());
        for (const auto& src_material : asset.materials) {
            Material material;
            material.SetBaseColorFactor(glm::vec4(src_material.pbrData.baseColorFactor.x(),
                                                  src_material.pbrData.baseColorFactor.y(),
                                                  src_material.pbrData.baseColorFactor.z(),
                                                  src_material.pbrData.baseColorFactor.w()));

            if (src_material.pbrData.baseColorTexture.has_value()) {
                TryLoadMaterialTexture(material,
                                       asset,
                                       src_material.pbrData.baseColorTexture->textureIndex,
                                       path.parent_path());
            }

            materials_.emplace_back(std::move(material));
        }
    }

    std::vector<std::size_t> mesh_remap(asset.meshes.size(), std::numeric_limits<std::size_t>::max());
    meshes_.reserve(asset.meshes.size());

    for (std::size_t mesh_idx = 0; mesh_idx < asset.meshes.size(); ++mesh_idx) {
        const auto& src_mesh = asset.meshes[mesh_idx];

        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<SubMesh> submeshes;

        for (const auto& primitive : src_mesh.primitives) {
            const GLenum primitive_mode = PrimitiveTypeToGL(primitive.type);
            if (primitive_mode == GL_NONE) {
                continue;
            }

            const auto position_it = primitive.findAttribute("POSITION");
            if (position_it == primitive.attributes.end()) {
                continue;
            }

            const auto& position_accessor = asset.accessors[position_it->accessorIndex];
            const std::size_t vertex_start = vertices.size();
            vertices.resize(vertex_start + position_accessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset,
                position_accessor,
                [&](fastgltf::math::fvec3 position, std::size_t vertex_idx) {
                    auto& vertex = vertices[vertex_start + vertex_idx];
                    vertex.position = glm::vec3(position.x(), position.y(), position.z());
                });

            if (const auto normal_it = primitive.findAttribute("NORMAL");
                normal_it != primitive.attributes.end()) {
                const auto& normal_accessor = asset.accessors[normal_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset,
                    normal_accessor,
                    [&](fastgltf::math::fvec3 normal, std::size_t vertex_idx) {
                        auto& vertex = vertices[vertex_start + vertex_idx];
                        vertex.normal = glm::normalize(glm::vec3(normal.x(), normal.y(), normal.z()));
                    });
            }

            if (const auto uv_it = primitive.findAttribute("TEXCOORD_0");
                uv_it != primitive.attributes.end()) {
                const auto& uv_accessor = asset.accessors[uv_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset,
                    uv_accessor,
                    [&](fastgltf::math::fvec2 uv, std::size_t vertex_idx) {
                        auto& vertex = vertices[vertex_start + vertex_idx];
                        vertex.texcoord = glm::vec2(uv.x(), uv.y());
                    });
            }

            const std::size_t index_offset = indices.size();
            std::size_t index_count = 0;

            if (primitive.indicesAccessor.has_value()) {
                const auto& index_accessor = asset.accessors[primitive.indicesAccessor.value()];
                index_count = index_accessor.count;

                std::vector<std::uint32_t> local_indices(index_count);
                fastgltf::copyFromAccessor<std::uint32_t>(asset, index_accessor, local_indices.data());

                indices.resize(index_offset + index_count);
                for (std::size_t i = 0; i < index_count; ++i) {
                    indices[index_offset + i] =
                        static_cast<std::uint32_t>(local_indices[i] + vertex_start);
                }
            } else {
                index_count = position_accessor.count;
                indices.resize(index_offset + index_count);
                for (std::size_t i = 0; i < index_count; ++i) {
                    indices[index_offset + i] = static_cast<std::uint32_t>(vertex_start + i);
                }
            }

            SubMesh submesh;
            submesh.index_offset = static_cast<std::uint32_t>(index_offset);
            submesh.index_count = static_cast<std::uint32_t>(index_count);
            submesh.primitive_mode = primitive_mode;
            submesh.material_index = primitive.materialIndex.value_or(0);
            if (submesh.material_index >= materials_.size()) {
                submesh.material_index = 0;
            }
            submeshes.emplace_back(submesh);
        }

        if (!submeshes.empty()) {
            mesh_remap[mesh_idx] = meshes_.size();
            meshes_.emplace_back(std::move(vertices), std::move(indices), std::move(submeshes));
        }
    }

    if (!asset.scenes.empty()) {
        std::size_t scene_index = asset.defaultScene.value_or(0);
        if (scene_index >= asset.scenes.size()) {
            scene_index = 0;
        }

        fastgltf::iterateSceneNodes(
            asset,
            scene_index,
            fastgltf::math::fmat4x4(),
            [&](fastgltf::Node& node, const fastgltf::math::fmat4x4& transform) {
                if (!node.meshIndex.has_value()) {
                    return;
                }

                const std::size_t gltf_mesh_index = node.meshIndex.value();
                if (gltf_mesh_index >= mesh_remap.size()) {
                    return;
                }

                const std::size_t mesh_index = mesh_remap[gltf_mesh_index];
                if (mesh_index == std::numeric_limits<std::size_t>::max()) {
                    return;
                }

                instances_.push_back(MeshInstance {mesh_index, ToGlmMat4(transform)});
            });
    }

    if (instances_.empty()) {
        instances_.reserve(meshes_.size());
        for (std::size_t i = 0; i < meshes_.size(); ++i) {
            instances_.push_back(MeshInstance {i, glm::mat4(1.0f)});
        }
    }

    loaded_ = !meshes_.empty();
    return loaded_;
}

void Model::Draw(const gl::Shader& shader, const glm::mat4& root_transform) const
{
    if (!loaded_) {
        return;
    }

    const GLint model_loc = shader.Uniform("u_Model");

    for (const auto& instance : instances_) {
        if (instance.mesh_index >= meshes_.size()) {
            continue;
        }

        const glm::mat4 model = root_transform * instance.transform;
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        meshes_[instance.mesh_index].Draw(materials_, shader);
    }
}

bool Model::IsLoaded() const
{
    return loaded_;
}

std::size_t Model::GetMeshCount() const
{
    return meshes_.size();
}

std::size_t Model::GetMaterialCount() const
{
    return materials_.size();
}

std::size_t Model::GetInstanceCount() const
{
    return instances_.size();
}

void Model::Clear()
{
    meshes_.clear();
    materials_.clear();
    instances_.clear();
    loaded_ = false;
}
