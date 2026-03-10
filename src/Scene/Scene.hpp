#pragma once

#include <string>
#include <string_view>

class Scene {
public:
    explicit Scene(std::string_view name)
        : name_(name)
    {}

    virtual ~Scene() = default;

    virtual void OnEnter() {}

    virtual void OnExit() {}

    virtual void OnUpdate(float dt) {}

    virtual void OnRender(float width, float height) = 0;

    virtual void OnRenderUI() {}

    const std::string& GetName() const
    {
        return name_;
    }

private:
    std::string name_;
};
