#include "Scene/Scenes/PBR/PBR.hpp"
#include "Scene/Scenes/PBR/PBRState.hpp"

#include <memory>

namespace {
constexpr const char* kPbrModelPath = "res/model/DamagedHelmet/DamagedHelmet.gltf";
}

PBR::PBR()
    : Scene("PBR"),
      state_(std::make_unique<PBRState>(kPbrModelPath))
{}

PBR::~PBR() = default;

void PBR::OnEnter()
{
    state_->OnEnter();
}

void PBR::OnExit()
{
    state_->OnExit();
}

void PBR::OnUpdate(float dt)
{
    state_->OnUpdate(dt);
}

void PBR::OnRender(float width, float height)
{
    state_->OnRender(width, height);
}

void PBR::OnRenderUI()
{
    state_->OnRenderUI();
}

void PBR::OnEvent(flux::Event& event)
{
    state_->OnEvent(event);
}
