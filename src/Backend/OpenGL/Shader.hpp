#pragma once

#include <glad/glad.h>
#include <string>

namespace gl {

// RAII Shader Program
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    bool Compile(const char* vert_src, const char* frag_src);
    bool CompileFromFile(const std::string& vert_path, const std::string& frag_path);

    void Bind() const;
    void Unbind() const;
    GLuint Id() const;
    GLint Uniform(const char* name) const;

private:
    GLuint id_ = 0;

    static std::string ReadFile(const std::string& path);
    static GLuint CompileStage(GLenum type, const char* src);
};

} // namespace gl
