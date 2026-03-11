#pragma once

#include <glm/glm.hpp>

enum class ProjectionType {
    Orthographic,
    Perspective
};

class Camera {
public:
    virtual ~Camera() = default;

    virtual void Update(float dt) {}

    virtual const glm::mat4& GetViewMatrix() const = 0;
    virtual const glm::mat4& GetProjectionMatrix() const = 0;

    glm::mat4 GetViewProjectionMatrix() const
    {
        return GetProjectionMatrix() * GetViewMatrix();
    }

    virtual ProjectionType GetProjectionType() const = 0;

protected:
    Camera() = default;
};
