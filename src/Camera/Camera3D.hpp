#pragma once

#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

class Camera3D : public Camera {
public:
    Camera3D(float fov = 45.0f, float aspect = 16.0f / 9.0f, float near_plane = 0.1f, float far_plane = 100.0f)
        : fov_(fov)
        , aspect_(aspect)
        , near_plane_(near_plane)
        , far_plane_(far_plane)
        , projection_type_(ProjectionType::Perspective)
    {
        RecalculateMatrices();
    }

    // Projection settings
    void SetPerspective(float fov, float aspect, float near_plane, float far_plane)
    {
        projection_type_ = ProjectionType::Perspective;
        fov_ = fov;
        aspect_ = aspect;
        near_plane_ = near_plane;
        far_plane_ = far_plane;
        RecalculateMatrices();
    }

    void SetOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
        projection_type_ = ProjectionType::Orthographic;
        ortho_left_ = left;
        ortho_right_ = right;
        ortho_bottom_ = bottom;
        ortho_top_ = top;
        near_plane_ = near_plane;
        far_plane_ = far_plane;
        RecalculateMatrices();
    }

    void SetProjectionType(ProjectionType type)
    {
        projection_type_ = type;
        RecalculateMatrices();
    }

    void SetAspectRatio(float aspect)
    {
        aspect_ = aspect;
        RecalculateMatrices();
    }

    // Camera transform
    void SetPosition(const glm::vec3& position)
    {
        position_ = position;
        RecalculateMatrices();
    }

    void SetRotation(float pitch, float yaw)
    {
        pitch_ = glm::clamp(pitch, -89.0f, 89.0f);
        yaw_ = yaw;
        RecalculateMatrices();
    }

    void SetTarget(const glm::vec3& target)
    {
        glm::vec3 direction = glm::normalize(target - position_);
        pitch_ = glm::degrees(asin(direction.y));
        yaw_ = glm::degrees(atan2(direction.z, direction.x));
        RecalculateMatrices();
    }

    // Getters
    const glm::vec3& GetPosition() const { return position_; }
    const glm::vec3& GetFront() const { return front_; }
    const glm::vec3& GetUp() const { return up_; }
    const glm::vec3& GetRight() const { return right_; }
    float GetPitch() const { return pitch_; }
    float GetYaw() const { return yaw_; }
    float GetFOV() const { return fov_; }

    const glm::mat4& GetViewMatrix() const override { return view_matrix_; }
    const glm::mat4& GetProjectionMatrix() const override { return projection_matrix_; }
    ProjectionType GetProjectionType() const override { return projection_type_; }

private:
    void RecalculateMatrices()
    {
        // Update camera vectors
        glm::vec3 front;
        front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        front.y = sin(glm::radians(pitch_));
        front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        front_ = glm::normalize(front);

        right_ = glm::normalize(glm::cross(front_, world_up_));
        up_ = glm::normalize(glm::cross(right_, front_));

        // View matrix
        view_matrix_ = glm::lookAt(position_, position_ + front_, up_);

        // Projection matrix
        if (projection_type_ == ProjectionType::Perspective) {
            projection_matrix_ = glm::perspective(glm::radians(fov_), aspect_, near_plane_, far_plane_);
        } else {
            projection_matrix_ = glm::ortho(ortho_left_, ortho_right_, ortho_bottom_, ortho_top_, near_plane_, far_plane_);
        }
    }

    // Projection parameters
    ProjectionType projection_type_;
    float fov_;
    float aspect_;
    float near_plane_;
    float far_plane_;

    // Orthographic bounds
    float ortho_left_ = -10.0f;
    float ortho_right_ = 10.0f;
    float ortho_bottom_ = -10.0f;
    float ortho_top_ = 10.0f;

    // Camera transform
    glm::vec3 position_ = glm::vec3(0.0f, 0.0f, 3.0f);
    float pitch_ = 0.0f;
    float yaw_ = -90.0f;

    // Camera vectors
    glm::vec3 front_ = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);

    // Matrices
    glm::mat4 view_matrix_ = glm::mat4(1.0f);
    glm::mat4 projection_matrix_ = glm::mat4(1.0f);
};
