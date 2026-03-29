#pragma once
#include <string>
#include <functional>
#include <future>
#include <chrono>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <json/json.h>

enum class Priority : int {
    Critical = 0,
    High = 1,
    Normal = 2,
    Low = 3
};

struct QueuedRequest {
    Priority priority;
    std::string request_id;
    std::string key_alias;
    std::chrono::steady_clock::time_point enqueued_at;
    std::chrono::milliseconds timeout;
    std::function<void()> execute;
    std::promise<bool> completion;
};

class RequestQueue {
public:
    explicit RequestQueue(size_t max_size = 1000,
                         size_t max_concurrency = 4,
                         size_t worker_threads = 4) noexcept;
    ~RequestQueue();

    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    struct EnqueueResult {
        bool accepted;
        size_t position;
        std::future<bool> completion;
    };

    [[nodiscard]] EnqueueResult enqueue(Priority priority,
                                       std::chrono::milliseconds timeout,
                                       std::function<void()> task,
                                       const std::string& request_id = "",
                                       const std::string& key_alias = "");

    [[nodiscard]] size_t pending() const noexcept;
    [[nodiscard]] size_t active() const noexcept;
    [[nodiscard]] Json::Value stats() const;

    void shutdown();
    void clear_pending();

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    void set_enabled(bool v) noexcept { enabled_ = v; }

private:
    struct PriorityCompare {
        bool operator()(const QueuedRequest& a, const QueuedRequest& b) const {
            if (static_cast<int>(a.priority) != static_cast<int>(b.priority))
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            return a.enqueued_at > b.enqueued_at;
        }
    };

    std::vector<QueuedRequest> queue_;
    PriorityCompare comp_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::jthread> workers_;
    std::atomic<bool> running_{true};
    std::atomic<bool> enabled_{false};
    std::atomic<size_t> active_count_{0};
    size_t max_size_;
    size_t max_concurrency_;
    size_t worker_threads_ = 4;
    std::counting_semaphore<> concurrency_sem_;

    std::atomic<int64_t> total_enqueued_{0};
    std::atomic<int64_t> total_processed_{0};
    std::atomic<int64_t> total_rejected_{0};
    std::atomic<int64_t> total_timed_out_{0};
    std::atomic<int64_t> total_wait_ms_{0};

    void worker_loop();
    bool pop_next(QueuedRequest& out);
};
