#pragma once
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>

struct ChunkQueue {
    std::queue<std::string> chunks;
    std::mutex mtx;
    std::condition_variable cv;
    bool finished = false;
    std::atomic<bool> cancelled{false};

    void push(std::string chunk) {
        std::lock_guard lock(mtx);
        chunks.push(std::move(chunk));
        cv.notify_one();
    }

    void finish() noexcept {
        std::lock_guard lock(mtx);
        finished = true;
        cv.notify_all();
    }

    void cancel() noexcept {
        cancelled.store(true, std::memory_order_release);
        std::lock_guard lock(mtx);
        finished = true;
        cv.notify_all();
    }

    [[nodiscard]] std::optional<std::string> pop() {
        std::unique_lock lock(mtx);
        cv.wait(lock, [this] { return !chunks.empty() || finished; });
        if (chunks.empty()) return std::nullopt;
        auto chunk = std::move(chunks.front());
        chunks.pop();
        return chunk;
    }
};
