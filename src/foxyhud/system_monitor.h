#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace foxy {

// Live CPU / GPU / RAM usage for the watermark, sampled once per second on a
// background thread (the same numbers Task Manager shows on Windows).
// A value is -1 when it can't be read on this system.
class SystemMonitor {
public:
    struct Stats {
        float cpu_percent  = -1.0f;
        float gpu_percent  = -1.0f;
        float ram_used_gb  = -1.0f;
        float ram_total_gb = -1.0f;
    };

    SystemMonitor() = default;
    ~SystemMonitor();
    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    void  Start();  // starts the sampling thread (no-op if running)
    void  Stop();   // stops and joins it
    bool  IsRunning() const { return thread_.joinable(); }
    Stats Get() const;

private:
    void Run();

    std::thread             thread_;
    mutable std::mutex      mutex_;
    std::condition_variable wake_;
    bool                    stop_ = false;
    Stats                   stats_;
};

} // namespace foxy
