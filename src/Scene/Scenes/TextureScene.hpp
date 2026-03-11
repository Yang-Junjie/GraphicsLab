#pragma once

#include "../Scene.hpp"
#include "Backend/OpenGL/GL.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class TextureScene : public Scene {
public:
    TextureScene();
    ~TextureScene() override;

    void OnEnter() override;
    void OnExit() override;
    void OnRender(float width, float height) override;
    void OnRenderUI() override;

private:
    struct ImageInfo {
        std::string name;
        std::string path;
        GLuint texture = 0;
        int width = 0;
        int height = 0;
    };

    gl::VertexArray vao_;
    gl::Buffer vbo_;
    gl::Buffer ebo_;
    gl::Shader shader_;

    GLint proj_loc_ = -1;
    GLint model_loc_ = -1;
    GLint tex_loc_ = -1;

    std::vector<ImageInfo> images_;
    int current_image_ = 0;
    float scale_ = 1.0f;
    bool initialized_ = false;

    void InitResources();
    void LoadImages();
    static GLuint LoadTexture(const std::string& path, int& w, int& h);
};
