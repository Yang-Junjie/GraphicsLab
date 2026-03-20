#include "Backend/OpenGL/Buffer.hpp"
#include "ComputeShaderScene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

ComputeShaderScene::ComputeShaderScene()
    : Scene("compute shader")
{
    for (int i = 0; i < N_; i++) {
        data_[i] = static_cast<float>(i);
    }
}

void ComputeShaderScene::OnEnter()
{

    if (!compute_shader_.CompileComputeFromFile("shaders/ComputeShaderScene/compute.comp")) {
        return;
    }

    glCreateBuffers(1, &ssbo_);
    glNamedBufferStorage(ssbo_, sizeof(data_), data_, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_);

    compute_shader_.Bind();
    glDispatchCompute((N_ + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    glGetNamedBufferSubData(ssbo_, 0, sizeof(data_), data_);

    ready_ = true;
}

void ComputeShaderScene::OnExit()
{
    if (ssbo_) {
        glDeleteBuffers(1, &ssbo_);
        ssbo_ = 0;
    }
    compute_shader_ = gl::Shader();
    ready_ = false;
}

void ComputeShaderScene::OnRender(float width, float height) {}

void ComputeShaderScene::OnRenderUI()
{
    if (!ready_) {
        return;
    }

    for (int i = 0; i < N_; i++) {
        ImGui::Text("%f", data_[i]);
    }
}
