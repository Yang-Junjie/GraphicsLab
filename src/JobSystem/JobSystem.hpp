#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace job {

// ---- Counter ---------------------------------------------------------------
// Atomic counter with blocking Wait(). Submit() increments; workers decrement
// on completion.  Wait() blocks until the count reaches zero.
class Counter {
public:
    void Increment(int n = 1)
    {
        count_.fetch_add(n, std::memory_order_release);
    }

    void Decrement()
    {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            std::lock_guard<std::mutex> lk(mtx_);
            cv_.notify_all();
        }
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this] {
            return count_.load(std::memory_order_acquire) == 0;
        });
    }

private:
    std::atomic<int> count_{0};
    std::mutex mtx_;
    std::condition_variable cv_;
};

// ---- JobSystem -------------------------------------------------------------
class JobSystem {
public:
    // thread_count == 0  =>  hardware_concurrency - 1  (at least 1)
    explicit JobSystem(int thread_count = 0);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Submit a single task.  If counter is non-null it is incremented before
    // enqueue and decremented after the task completes.
    void Submit(std::function<void()> task, Counter* counter = nullptr);

    // Parallel-for over [begin, end).  Each index is processed exactly once.
    // If counter is non-null, Wait() on it will block until all chunks finish.
    template <typename Func>
    void ParallelFor(int begin, int end, Func&& func, Counter* counter = nullptr)
    {
        if (begin >= end) {
            return;
        }

        int range = end - begin;
        int num_workers = static_cast<int>(workers_.size());
        int chunks = num_workers + 1; // +1 for caller
        int chunk_size = (range + chunks - 1) / chunks;
        if (chunk_size < 1) {
            chunk_size = 1;
        }

        // Submit to workers
        for (int i = begin; i < end - chunk_size; i += chunk_size) {
            int blk_end = std::min(i + chunk_size, end);
            Submit(
                [&func, i, blk_end]() {
                    for (int j = i; j < blk_end; ++j) {
                        func(j);
                    }
                },
                counter);
        }

        // Last chunk runs on caller thread
        int last_begin = begin + ((range - 1) / chunk_size) * chunk_size;
        for (int j = last_begin; j < end; ++j) {
            func(j);
        }
    }

    int ThreadCount() const
    {
        return static_cast<int>(workers_.size());
    }

private:
    struct Job {
        std::function<void()> task;
        Counter* counter = nullptr;
    };

    std::vector<std::thread> workers_;
    std::queue<Job> queue_;
    std::mutex queue_mtx_;
    std::condition_variable queue_cv_;
    bool shutdown_ = false;

    void WorkerLoop();
};

} // namespace job
