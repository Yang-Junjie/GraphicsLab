#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace sim {

struct Particle {
    glm::vec2 pos;
    glm::vec2 vel;
};

class ParticleSystem {
public:
    void Init(int count, float radius, float width, float height);
    void Update(float dt, float width, float height);
    void Reset(float width, float height);

    const std::vector<Particle>& GetParticles() const { return particles_; }
    int GetCount() const { return static_cast<int>(particles_.size()); }
    float GetRadius() const { return radius_; }

    float gravity = 500.0f;
    float restitution = 0.5f;
    float damping = 0.998f;
    int solver_iterations = 4;
    int sub_steps = 2;

private:
    std::vector<Particle> particles_;
    float radius_ = 2.0f;
    int num_threads_ = 1;

    // Spatial hash grid (counting-sort, allocation-free after init)
    std::vector<int> cell_start_;
    std::vector<int> cell_count_;
    std::vector<int> sorted_ids_;
    int grid_cols_ = 0, grid_rows_ = 0;
    float cell_size_ = 0.0f;

    // Per-particle correction buffers
    std::vector<glm::vec2> delta_pos_;
    std::vector<glm::vec2> delta_vel_;

    void BuildGrid(float width, float height);
    void ResolveCollisions();
    int CellIndex(float x, float y) const;
};

} // namespace sim
