#pragma once

#include <string>
#include <string_view>

namespace flux { class Event; }

enum class RendererAPI {
    None = 0,
    OpenGL = 1,
    // Future: DirectX, Vulkan, Metal, RHI etc.
};

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

    virtual void OnEvent(flux::Event& event) {}

    const std::string& GetName() const
    {
        return name_;
    }

    RendererAPI GetRendererAPI() const
    {
        return api_;
    }

    void SetRendererAPI(RendererAPI api)
    {
        api_ = api;
    }

    static const char* RendererAPIToString(RendererAPI api)
    {
        switch (api) {
        case RendererAPI::None:
            return "None";
        case RendererAPI::OpenGL:
            return "OpenGL";
        default:
            return "Unknown";
        }
    }

private:
    std::string name_;
    RendererAPI api_ = RendererAPI::OpenGL; // For future multi-API support
};
