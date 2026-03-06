#include "ParticleSystem.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

namespace sim {

// ---- Multi-threaded parallel for ----
// Only used for collision detection (the dominant bottleneck).
// Main thread executes first chunk; worker threads handle the rest.

template <typename F>
static void RunParallel(int count, int num_threads, F&& func) {
    if (count <= 0) return;
    if (num_threads <= 1 || count < 256) {
        for (int i = 0; i < count; i++) func(i);
        return;
    }

    int chunk = (count + num_threads - 1) / num_threads;
    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);

    for (int t = 1; t < num_threads; t++) {
        int s = t * chunk;
        int e = std::min(s + chunk, count);
        if (s >= e) break;
        threads.emplace_back([&func, s, e]() {
            for (int i = s; i < e; i++) func(i);
        });
    }

    int first_end = std::min(chunk, count);
    for (int i = 0; i < first_end; i++) func(i);

    for (auto& t : threads) t.join();
}

// ---- Init / Reset ----

void ParticleSystem::Init(int count, float radius, float width, float height) {
    radius_ = radius;
    num_threads_ = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    particles_.resize(count);
    delta_pos_.resize(count);
    delta_vel_.resize(count);
    sorted_ids_.resize(count);
    Reset(width, height);
}

void ParticleSystem::Reset(float width, float height) {
    int n = static_cast<int>(particles_.size());
    float spacing = radius_ * 2.1f;
    int cols = std::max(1, static_cast<int>(width * 0.5f / spacing));
    float start_x = width * 0.25f;
    float start_y = height * 0.05f;

    for (int i = 0; i < n; i++) {
        int r = i / cols;
        int c = i % cols;
        particles_[i].pos = {start_x + c * spacing, start_y + r * spacing};
        particles_[i].vel = {0.0f, 0.0f};
    }
}

// ---- Spatial Hash Grid (counting sort) ----

int ParticleSystem::CellIndex(float x, float y) const {
    int cx = std::clamp(static_cast<int>(x / cell_size_), 0, grid_cols_ - 1);
    int cy = std::clamp(static_cast<int>(y / cell_size_), 0, grid_rows_ - 1);
    return cy * grid_cols_ + cx;
}

void ParticleSystem::BuildGrid(float width, float height) {
    cell_size_ = radius_ * 2.0f;
    grid_cols_ = std::max(1, static_cast<int>(width / cell_size_) + 1);
    grid_rows_ = std::max(1, static_cast<int>(height / cell_size_) + 1);
    int num_cells = grid_cols_ * grid_rows_;
    int n = static_cast<int>(particles_.size());

    if (static_cast<int>(cell_start_.size()) != num_cells + 1)
        cell_start_.resize(num_cells + 1);
    if (static_cast<int>(cell_count_.size()) != num_cells)
        cell_count_.resize(num_cells);

    // Pass 1: count particles per cell
    std::fill(cell_count_.begin(), cell_count_.end(), 0);
    for (int i = 0; i < n; i++)
        cell_count_[CellIndex(particles_[i].pos.x, particles_[i].pos.y)]++;

    // Prefix sum -> cell_start_
    cell_start_[0] = 0;
    for (int i = 0; i < num_cells; i++)
        cell_start_[i + 1] = cell_start_[i] + cell_count_[i];

    // Pass 2: place particle indices (reuse cell_count_ as write cursor)
    std::fill(cell_count_.begin(), cell_count_.end(), 0);
    for (int i = 0; i < n; i++) {
        int cell = CellIndex(particles_[i].pos.x, particles_[i].pos.y);
        sorted_ids_[cell_start_[cell] + cell_count_[cell]++] = i;
    }
}

// ---- Collision Detection & Response (Gather pattern, thread-safe) ----

void ParticleSystem::ResolveCollisions() {
    int n = static_cast<int>(particles_.size());
    float min_dist = radius_ * 2.0f;
    float min_dist2 = min_dist * min_dist;
    float impulse_scale = (1.0f + restitution) * 0.5f;

    RunParallel(n, num_threads_, [&](int i) {
        glm::vec2 dp(0.0f);
        glm::vec2 dv(0.0f);

        const glm::vec2 pi_pos = particles_[i].pos;
        const glm::vec2 pi_vel = particles_[i].vel;
        int cx = std::clamp(static_cast<int>(pi_pos.x / cell_size_), 0, grid_cols_ - 1);
        int cy = std::clamp(static_cast<int>(pi_pos.y / cell_size_), 0, grid_rows_ - 1);

        int y0 = std::max(0, cy - 1), y1 = std::min(grid_rows_ - 1, cy + 1);
        int x0 = std::max(0, cx - 1), x1 = std::min(grid_cols_ - 1, cx + 1);

        for (int ny = y0; ny <= y1; ny++) {
            for (int nx = x0; nx <= x1; nx++) {
                int cell = ny * grid_cols_ + nx;
                int start = cell_start_[cell];
                int end = cell_start_[cell + 1];

                for (int k = start; k < end; k++) {
                    int j = sorted_ids_[k];
                    if (j == i) continue;

                    glm::vec2 diff = pi_pos - particles_[j].pos;
                    float dist2 = glm::dot(diff, diff);

                    if (dist2 < min_dist2 && dist2 > 1e-8f) {
                        float dist = std::sqrt(dist2);
                        glm::vec2 normal = diff / dist;
                        float overlap = min_dist - dist;

                        dp += normal * (overlap * 0.5f);

                        float vn = glm::dot(pi_vel - particles_[j].vel, normal);
                        if (vn < 0.0f)
                            dv -= normal * (vn * impulse_scale);
                    }
                }
            }
        }

        delta_pos_[i] = dp;
        delta_vel_[i] = dv;
    });

    // Apply corrections (sequential, trivially fast)
    for (int i = 0; i < n; i++) {
        particles_[i].pos += delta_pos_[i];
        particles_[i].vel += delta_vel_[i];
    }
}

// ---- Main Update Loop ----

void ParticleSystem::Update(float dt, float width, float height) {
    int n = static_cast<int>(particles_.size());
    if (n == 0) return;

    float sub_dt = dt / static_cast<float>(sub_steps);

    for (int step = 0; step < sub_steps; step++) {
        // Lagrangian integration: track each particle through space
        for (int i = 0; i < n; i++) {
            particles_[i].vel.y += gravity * sub_dt;
            particles_[i].pos += particles_[i].vel * sub_dt;
        }

        // Iterative collision resolution
        for (int iter = 0; iter < solver_iterations; iter++) {
            BuildGrid(width, height);
            ResolveCollisions();
        }

        // Boundary enforcement
        for (int i = 0; i < n; i++) {
            auto& p = particles_[i];
            if (p.pos.x < radius_)         { p.pos.x = radius_;         if (p.vel.x < 0) p.vel.x *= -restitution; }
            if (p.pos.x > width - radius_) { p.pos.x = width - radius_; if (p.vel.x > 0) p.vel.x *= -restitution; }
            if (p.pos.y < radius_)         { p.pos.y = radius_;         if (p.vel.y < 0) p.vel.y *= -restitution; }
            if (p.pos.y > height - radius_){ p.pos.y = height - radius_; if (p.vel.y > 0) p.vel.y *= -restitution; }
        }
    }

    // Frame-level damping
    for (int i = 0; i < n; i++)
        particles_[i].vel *= damping;
}

} // namespace sim
