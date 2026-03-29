#include "request_queue.h"
#include "logger.h"
#include <algorithm>

RequestQueue::RequestQueue(size_t max_size, size_t max_concurrency,
                           size_t worker_threads) noexcept
    : max_size_(max_size)
    , max_concurrency_(max_concurrency)
    , worker_threads_(worker_threads)
    , concurrency_sem_(static_cast<std::ptrdiff_t>(max_concurrency > 0 ? max_concurrency : 1)) {
    for (size_t i = 0; i < worker_threads_; ++i)
        workers_.emplace_back(&RequestQueue::worker_loop, this);
}

RequestQueue::~RequestQueue() {
    shutdown();
}

RequestQueue::EnqueueResult RequestQueue::enqueue(Priority priority,
                                                  std::chrono::milliseconds timeout,
                                                  std::function<void()> task,
                                                  const std::string& request_id,
                                                  const std::string& key_alias) {
    if (!enabled_) {
        task();
        std::promise<bool> p;
        p.set_value(true);
        return EnqueueResult{.accepted = true, .position = 0, .completion = p.get_future()};
    }

    std::unique_lock lock(mtx_);
    if (queue_.size() >= max_size_) {
        total_rejected_++;
        std::promise<bool> p;
        p.set_value(false);
        return EnqueueResult{.accepted = false, .position = 0, .completion = p.get_future()};
    }

    QueuedRequest qr;
    qr.priority = priority;
    qr.request_id = request_id;
    qr.key_alias = key_alias;
    qr.enqueued_at = std::chrono::steady_clock::now();
    qr.timeout = timeout;
    qr.execute = std::move(task);

    auto fut = qr.completion.get_future();
    size_t pos = queue_.size() + 1;
    queue_.push_back(std::move(qr));
    std::push_heap(queue_.begin(), queue_.end(), comp_);
    total_enqueued_++;
    cv_.notify_one();

    return EnqueueResult{.accepted = true, .position = pos, .completion = std::move(fut)};
}

bool RequestQueue::pop_next(QueuedRequest& out) {
    if (queue_.empty()) return false;
    std::pop_heap(queue_.begin(), queue_.end(), comp_);
    out = std::move(queue_.back());
    queue_.pop_back();
    return true;
}

void RequestQueue::worker_loop() {
    while (running_) {
        QueuedRequest qr;
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this] {
                return !running_ || !queue_.empty();
            });
            if (!running_) break;
            if (!pop_next(qr)) continue;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - qr.enqueued_at).count();
        if (elapsed > qr.timeout.count()) {
            total_timed_out_++;
            qr.completion.set_value(false);
            continue;
        }

        concurrency_sem_.acquire();
        active_count_++;
        total_wait_ms_ += elapsed;

        bool ok = true;
        try {
            if (qr.execute) qr.execute();
        } catch (const std::exception& e) {
            Json::Value f;
            f["request_id"] = qr.request_id;
            f["error"] = e.what();
            Logger::error("queue_task_error", f);
            ok = false;
        }

        active_count_--;
        concurrency_sem_.release();
        total_processed_++;
        qr.completion.set_value(ok);
    }
}

size_t RequestQueue::pending() const noexcept {
    std::lock_guard lock(mtx_);
    return queue_.size();
}

size_t RequestQueue::active() const noexcept {
    return active_count_.load();
}

Json::Value RequestQueue::stats() const {
    Json::Value s;
    std::lock_guard lock(mtx_);
    s["pending"] = static_cast<Json::Int64>(queue_.size());
    s["active"] = static_cast<Json::Int64>(active_count_.load());
    s["max_size"] = static_cast<Json::Int64>(max_size_);
    s["max_concurrency"] = static_cast<Json::Int64>(max_concurrency_);
    s["total_enqueued"] = static_cast<Json::Int64>(total_enqueued_.load());
    s["total_processed"] = static_cast<Json::Int64>(total_processed_.load());
    s["total_rejected"] = static_cast<Json::Int64>(total_rejected_.load());
    s["total_timed_out"] = static_cast<Json::Int64>(total_timed_out_.load());
    int64_t processed = total_processed_.load();
    int64_t wait_ms = total_wait_ms_.load();
    s["avg_wait_ms"] = (processed > 0)
        ? static_cast<Json::Int64>(wait_ms / processed)
        : Json::Value(0);
    return s;
}

void RequestQueue::shutdown() {
    running_ = false;
    cv_.notify_all();
    workers_.clear();
}

void RequestQueue::clear_pending() {
    std::lock_guard lock(mtx_);
    while (!queue_.empty()) {
        QueuedRequest qr;
        pop_next(qr);
        qr.completion.set_value(false);
    }
}
