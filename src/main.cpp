#include "gateway.h"
#include <csignal>
#include <iostream>
#include <atomic>

static std::atomic<bool> g_shutdown_requested{false};
static HermesGateway* g_gateway = nullptr;

extern "C" void shutdown_handler(int) {
    g_shutdown_requested.store(true, std::memory_order_relaxed);
}

int main() {
    try {
        HermesGateway gateway;
        g_gateway = &gateway;

        std::signal(SIGINT, shutdown_handler);
        std::signal(SIGTERM, shutdown_handler);

        std::thread shutdown_watcher([&] {
            while (!g_shutdown_requested.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            gateway.stop();
        });

        gateway.start();
        g_shutdown_requested.store(true);
        if (shutdown_watcher.joinable()) shutdown_watcher.join();

        Logger::info("gateway_stopped");
    } catch (const std::exception& e) {
        Json::Value f;
        f["message"] = e.what();
        Logger::error("fatal", f);
        return 1;
    }
    return 0;
}
