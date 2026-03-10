#include "BasicShapesScene.hpp"

#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <string>

// ---- helpers (file-local) --------------------------------------------------

static std::string ReadFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
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
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint LinkProgram(const char* vert_path, const char* frag_path)
{
    std::string vs_src = ReadFile(vert_path);
    std::string fs_src = ReadFile(frag_path);
    if (vs_src.empty() || fs_src.empty()) return 0;

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
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ---- BasicShapesScene ------------------------------------------------------

BasicShapesScene::BasicShapesScene()
    : Scene("Basic Shapes")
{}

void BasicShapesScene::OnEnter()
{
    program_ = LinkProgram("shaders/basic.vert", "shaders/basic.frag");
    proj_loc_ = glGetUniformLocation(program_, "u_Proj");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // layout: vec2 pos + vec4 color  (stride = 6 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void BasicShapesScene::OnExit()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(program_);
    vbo_ = 0;
    vao_ = 0;
    program_ = 0;
}

void BasicShapesScene::OnRender(float width, float height)
{
    float cx = width * 0.5f;
    float cy = height * 0.5f;
    float r  = std::min(width, height) * 0.3f;

    constexpr float sin60 = 0.866025f;
    constexpr float cos60 = 0.5f;

    // clang-format off
    float vertices[] = {
        //  position                         color (RGBA)
        cx,              cy - r,           1.0f, 0.2f, 0.2f, 1.0f,  // top – red
        cx - r * sin60,  cy + r * cos60,   0.2f, 1.0f, 0.2f, 1.0f,  // bottom-left – green
        cx + r * sin60,  cy + r * cos60,   0.2f, 0.4f, 1.0f, 1.0f,  // bottom-right – blue
    };
    // clang-format on

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glm::mat4 proj = glm::ortho(0.0f, width, height, 0.0f);

    glUseProgram(program_);
    glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);
}
