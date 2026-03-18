#include "Scene/Scenes/AdvanceLighting/AdvanceLighting.hpp"

#include "Scene/Scenes/AdvanceLighting/AdvanceLightingState.hpp"

#include <memory>

AdvanceLighting::AdvanceLighting()
    : Scene("AdvanceLighting")
    , state_(std::make_unique<AdvanceLightingState>())
{}

AdvanceLighting::~AdvanceLighting() = default;

void AdvanceLighting::OnEnter()
{
    state_->OnEnter();
}

void AdvanceLighting::OnExit()
{
    state_->OnExit();
}

void AdvanceLighting::OnUpdate(float dt)
{
    state_->OnUpdate(dt);
}

void AdvanceLighting::OnRender(float width, float height)
{
    state_->OnRender(width, height);
}

void AdvanceLighting::OnRenderUI()
{
    state_->OnRenderUI();
}

void AdvanceLighting::OnEvent(flux::Event& event)
{
    state_->OnEvent(event);
}
