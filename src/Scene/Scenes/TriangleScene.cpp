#include "TriangleScene.hpp"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

// ---- helpers ------------------------------------------------------

static std::string ReadFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint CompileStage(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1'024];
        glGetShaderInfoLog(s, 1'024, nullptr, log);
        std::cerr << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint LinkProgram(const char* vert_path, const char* frag_path)
{
    std::string vs_src = ReadFile(vert_path);
    std::string fs_src = ReadFile(frag_path);
    if (vs_src.empty() || fs_src.empty()) {
        return 0;
    }

    GLuint vs = CompileStage(GL_VERTEX_SHADER, vs_src.c_str());
    GLuint fs = CompileStage(GL_FRAGMENT_SHADER, fs_src.c_str());
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1'024];
        glGetProgramInfoLog(prog, 1'024, nullptr, log);
        std::cerr << log << std::endl;
        glDeleteProgram(prog);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---- TriangleScene -------------------------------------------------

TriangleScene::TriangleScene()
    : Scene("Basic Shapes")
{}

void TriangleScene::OnEnter()
{
    program_ = LinkProgram("shaders/TriangleScene/basic.vert", "shaders/TriangleScene/basic.frag");

    // clang-format off
    float vertices[] = {
         0.0f,  0.5f, 0.0f,  1,0,0,
        -0.5f, -0.5f, 0.0f,  0,1,0,
         0.5f, -0.5f, 0.0f,  0,0,1
    };
    // clang-format on

    // ---- 创建 VBO ----
    glCreateBuffers(1, &vbo_);
    glNamedBufferData(vbo_, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // ---- 创建 VAO ----
    glCreateVertexArrays(1, &vao_);
    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(float) * 6);

    glEnableVertexArrayAttrib(vao_, 0); // position
    glEnableVertexArrayAttrib(vao_, 1); // color

    glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);

    glVertexArrayAttribBinding(vao_, 0, 0);
    glVertexArrayAttribBinding(vao_, 1, 0);

    // ---- 创建间接绘制 buffer ----
    struct DrawArraysIndirectCommand {
        GLuint count;         // 顶点数量
        GLuint instanceCount; // 实例数量
        GLuint first;         // 起始顶点
        GLuint baseInstance;  // baseInstance
    };

    DrawArraysIndirectCommand cmd = {3, 1, 0, 0}; // 三角形, 单实例

    glCreateBuffers(1, &indirectBuffer_);
    glNamedBufferData(indirectBuffer_, sizeof(cmd), &cmd, GL_STATIC_DRAW);
}

void TriangleScene::OnExit()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &indirectBuffer_);
    glDeleteProgram(program_);
    vbo_ = vao_ = indirectBuffer_ = program_ = 0;
}

void TriangleScene::OnRender(float width, float height)
{
    glUseProgram(program_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer_);

    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, 1, 0);

    glUseProgram(0);
}
