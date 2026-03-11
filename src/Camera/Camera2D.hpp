#pragma once

#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>


class Camera2D : public Camera {
public:
    /// Centered orthographic camera. size = half-height of the view.
    explicit Camera2D(float size = 5.0f, float aspect = 16.0f / 9.0f)
        : size_(size)
        , aspect_(aspect)
    {
        RecalculateMatrices();
    }

    void SetSize(float size)
    {
        size_ = glm::max(size, 0.01f);
        RecalculateMatrices();
    }

    void SetAspectRatio(float aspect)
    {
        aspect_ = aspect;
        RecalculateMatrices();
    }

    void SetPosition(const glm::vec2& position)
    {
        position_ = position;
        RecalculateMatrices();
    }

    void SetRotation(float degrees)
    {
        rotation_ = degrees;
        RecalculateMatrices();
    }

    void SetZoom(float zoom)
    {
        zoom_ = glm::max(zoom, 0.01f);
        RecalculateMatrices();
    }

    const glm::vec2& GetPosition() const { return position_; }
    float GetRotation() const { return rotation_; }
    float GetZoom() const { return zoom_; }
    float GetSize() const { return size_; }

    const glm::mat4& GetViewMatrix() const override { return view_matrix_; }
    const glm::mat4& GetProjectionMatrix() const override { return projection_matrix_; }
    ProjectionType GetProjectionType() const override { return ProjectionType::Orthographic; }

private:
    void RecalculateMatrices()
    {
        float half_h = size_ / zoom_;
        float half_w = half_h * aspect_;

        projection_matrix_ = glm::ortho(-half_w, half_w, -half_h, half_h, -1.0f, 1.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position_, 0.0f));
        transform = glm::rotate(transform, glm::radians(rotation_), glm::vec3(0, 0, 1));
        view_matrix_ = glm::inverse(transform);
    }

    float size_;
    float aspect_;
    glm::vec2 position_ = glm::vec2(0.0f);
    float rotation_ = 0.0f;
    float zoom_ = 1.0f;

    glm::mat4 view_matrix_ = glm::mat4(1.0f);
    glm::mat4 projection_matrix_ = glm::mat4(1.0f);
};
