#pragma once

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>

struct SubMesh {
    std::uint32_t index_offset = 0;
    std::uint32_t index_count = 0;
    GLenum primitive_mode = GL_TRIANGLES;
    std::size_t material_index = 0;
};
