#include "Shader.hpp"

#include <fstream>
#include <sstream>

namespace gl {

Shader::~Shader()
{
    if (id_) {
        glDeleteProgram(id_);
    }
}

Shader::Shader(Shader&& o) noexcept
    : id_(o.id_)
{
    o.id_ = 0;
}

Shader& Shader::operator=(Shader&& o) noexcept
{
    if (this != &o) {
        if (id_) {
            glDeleteProgram(id_);
        }
        id_ = o.id_;
        o.id_ = 0;
    }
    return *this;
}

bool Shader::Compile(const char* vert_src, const char* frag_src)
{
    GLuint vs = CompileStage(GL_VERTEX_SHADER, vert_src);
    GLuint fs = CompileStage(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

bool Shader::CompileCompute(const char* compute_src)
{
    GLuint cs = CompileStage(GL_COMPUTE_SHADER, compute_src);
    if (!cs) {
        glDeleteShader(cs);
        return false;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, cs);
    glLinkProgram(id_);
    glDeleteShader(cs);

    GLint ok;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

bool Shader::CompileFromFile(const std::string& vert_path, const std::string& frag_path)
{
    std::string vert_src = ReadFile(vert_path);
    std::string frag_src = ReadFile(frag_path);
    if (vert_src.empty() || frag_src.empty()) {
        return false;
    }
    return Compile(vert_src.c_str(), frag_src.c_str());
}

bool Shader::Compile(const char* vert_src,
                     const char* tcs_src,
                     const char* tes_src,
                     const char* frag_src)
{
    GLuint vs = CompileStage(GL_VERTEX_SHADER, vert_src);
    GLuint tc = CompileStage(GL_TESS_CONTROL_SHADER, tcs_src);
    GLuint te = CompileStage(GL_TESS_EVALUATION_SHADER, tes_src);
    GLuint fs = CompileStage(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !tc || !te || !fs) {
        glDeleteShader(vs);
        glDeleteShader(tc);
        glDeleteShader(te);
        glDeleteShader(fs);
        return false;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, tc);
    glAttachShader(id_, te);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    glDeleteShader(vs);
    glDeleteShader(tc);
    glDeleteShader(te);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

bool Shader::CompileFromFile(const std::string& vert_path,
                             const std::string& tcs_path,
                             const std::string& tes_path,
                             const std::string& frag_path)
{
    std::string vert_src = ReadFile(vert_path);
    std::string tcs_src = ReadFile(tcs_path);
    std::string tes_src = ReadFile(tes_path);
    std::string frag_src = ReadFile(frag_path);
    if (vert_src.empty() || tcs_src.empty() || tes_src.empty() || frag_src.empty()) {
        return false;
    }
    return Compile(vert_src.c_str(), tcs_src.c_str(), tes_src.c_str(), frag_src.c_str());
}

bool Shader::CompileComputeFromFile(const std::string& compute_path)
{
    std::string compute_src = ReadFile(compute_path);
    if (compute_src.empty()) {
        return false;
    }
    return CompileCompute(compute_src.c_str());
}

void Shader::Bind() const
{
    glUseProgram(id_);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

GLuint Shader::Id() const
{
    return id_;
}

GLint Shader::Uniform(const char* name) const
{
    return glGetUniformLocation(id_, name);
}

std::string Shader::ReadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::CompileStage(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

} // namespace gl
