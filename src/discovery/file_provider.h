#pragma once
#include "dynamic_config.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <sys/stat.h>

namespace discovery {

struct FileProviderConfig {
    std::vector<std::string> paths;
    bool use_watch = true;
    int poll_interval_seconds = 5;
};

class FileProvider : public IConfigProvider {
public:
    explicit FileProvider(FileProviderConfig config);
    ~FileProvider() override;

    [[nodiscard]] std::string name() const override { return "file"; }
    [[nodiscard]] DynamicConfigFragment provide() override;
    void watch(std::function<void()> on_change) override;
    void stop() override;
    [[nodiscard]] ProviderStatus status() const override;

private:
    FileProviderConfig config_;
    std::function<void()> on_change_;
    std::thread watch_thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex status_mu_;
    ProviderStatus status_;

    struct FileState {
        std::string path;
        time_t mtime = 0;
        off_t size = 0;
    };
    std::vector<FileState> tracked_files_;

    void poll_loop();
    bool detect_changes();
    [[nodiscard]] DynamicConfigFragment parse_yaml_file(const std::string& path) const;
    [[nodiscard]] DynamicConfigFragment parse_json_file(const std::string& path) const;
    [[nodiscard]] DynamicConfigFragment parse_file(const std::string& path) const;
    void scan_directory(const std::string& dir, std::vector<std::string>& out) const;

    static bool get_file_stat(const std::string& path, struct stat& st);
    static bool ends_with(const std::string& s, const std::string& suffix);
};

} // namespace discovery
