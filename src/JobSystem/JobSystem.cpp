#include "JobSystem/JobSystem.hpp"

#include <algorithm>

namespace job {

JobSystem::JobSystem(int thread_count)
{
    if (thread_count <= 0) {
        int hw = static_cast<int>(std::thread::hardware_concurrency());
        thread_count = std::max(1, hw - 1);
    }
    workers_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&JobSystem::WorkerLoop, this);
    }
}

JobSystem::~JobSystem()
{
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        shutdown_ = true;
    }
    queue_cv_.notify_all();
    for (auto& w : workers_) {
        w.join();
    }
}

void JobSystem::Submit(std::function<void()> task, Counter* counter)
{
    if (counter) {
        counter->Increment();
    }
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        queue_.push({std::move(task), counter});
    }
    queue_cv_.notify_one();
}

void JobSystem::WorkerLoop()
{
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> lk(queue_mtx_);
            queue_cv_.wait(lk, [this] {
                return shutdown_ || !queue_.empty();
            });
            if (shutdown_ && queue_.empty()) {
                return;
            }
            job = std::move(queue_.front());
            queue_.pop();
        }
        job.task();
        if (job.counter) {
            job.counter->Decrement();
        }
    }
}

} // namespace job
