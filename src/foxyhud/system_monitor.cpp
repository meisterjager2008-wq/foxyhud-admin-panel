#include "foxyhud/system_monitor.h"

#include <chrono>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <string>
#include <unordered_map>
#include <vector>
#elif defined(__linux__)
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace foxy {
namespace {

float Clamp100(double v) { return static_cast<float>(v < 0.0 ? 0.0 : (v > 100.0 ? 100.0 : v)); }

#if defined(_WIN32)

// Performance counters, the same source Task Manager uses.
class Sampler {
public:
    Sampler() {
        GetSystemTimes(&prev_idle_, &prev_kernel_, &prev_user_);
        if (PdhOpenQueryW(nullptr, 0, &query_) != ERROR_SUCCESS) {
            query_ = nullptr;
            return;
        }
        // "% Processor Utility" is what Task Manager shows for CPU (Windows 8+).
        if (PdhAddEnglishCounterW(query_, L"\\Processor Information(_Total)\\% Processor Utility", 0, &cpu_) != ERROR_SUCCESS)
            cpu_ = nullptr;
        // Per process and per engine GPU load (Windows 10 1709+).
        if (PdhAddEnglishCounterW(query_, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpu_) != ERROR_SUCCESS)
            gpu_ = nullptr;
        PdhCollectQueryData(query_);  // rate counters need a first sample to compare against
    }

    ~Sampler() {
        if (query_)
            PdhCloseQuery(query_);
    }

    SystemMonitor::Stats Sample() {
        SystemMonitor::Stats s;
        const bool collected = query_ && PdhCollectQueryData(query_) == ERROR_SUCCESS;

        const float cpu_from_times = CpuFromSystemTimes();  // also keeps the fallback's baseline fresh
        PDH_FMT_COUNTERVALUE value;
        if (collected && cpu_ &&
            PdhGetFormattedCounterValue(cpu_, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &value) == ERROR_SUCCESS &&
            (value.CStatus == PDH_CSTATUS_VALID_DATA || value.CStatus == PDH_CSTATUS_NEW_DATA))
            s.cpu_percent = Clamp100(value.doubleValue);
        else
            s.cpu_percent = cpu_from_times;

        if (collected && gpu_)
            s.gpu_percent = GpuFromCounters();

        MEMORYSTATUSEX mem = {};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem)) {
            constexpr double kGiB = 1024.0 * 1024.0 * 1024.0;
            s.ram_total_gb = static_cast<float>(mem.ullTotalPhys / kGiB);
            s.ram_used_gb = static_cast<float>((mem.ullTotalPhys - mem.ullAvailPhys) / kGiB);
        }
        return s;
    }

private:
    static ULONGLONG ToU64(const FILETIME& f) { return (static_cast<ULONGLONG>(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

    float CpuFromSystemTimes() {
        FILETIME idle, kernel, user;
        if (!GetSystemTimes(&idle, &kernel, &user))
            return -1.0f;
        const ULONGLONG d_idle = ToU64(idle) - ToU64(prev_idle_);
        const ULONGLONG d_busy = (ToU64(kernel) - ToU64(prev_kernel_)) + (ToU64(user) - ToU64(prev_user_));  // kernel includes idle
        prev_idle_ = idle;
        prev_kernel_ = kernel;
        prev_user_ = user;
        if (d_busy == 0)
            return -1.0f;
        return Clamp100(100.0 * (1.0 - static_cast<double>(d_idle) / static_cast<double>(d_busy)));
    }

    // Task Manager's "GPU" number: the busiest engine (3D, copy, video...),
    // summed over every process using that engine.
    float GpuFromCounters() {
        DWORD size = 0, count = 0;
        if (static_cast<DWORD>(PdhGetFormattedCounterArrayW(gpu_, PDH_FMT_DOUBLE, &size, &count, nullptr)) != PDH_MORE_DATA ||
            size == 0)
            return -1.0f;
        std::vector<unsigned char> buffer(size);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if (PdhGetFormattedCounterArrayW(gpu_, PDH_FMT_DOUBLE, &size, &count, items) != ERROR_SUCCESS)
            return -1.0f;

        // Instance names look like "pid_1234_luid_0x..._phys_0_eng_3_engtype_3D":
        // everything from "luid" on identifies the engine.
        std::unordered_map<std::wstring, double> per_engine;
        for (DWORD i = 0; i < count; ++i) {
            const DWORD status = items[i].FmtValue.CStatus;
            if (status != PDH_CSTATUS_VALID_DATA && status != PDH_CSTATUS_NEW_DATA)
                continue;
            const std::wstring name = items[i].szName;
            const size_t luid = name.find(L"luid_");
            per_engine[luid == std::wstring::npos ? name : name.substr(luid)] += items[i].FmtValue.doubleValue;
        }
        double busiest = 0.0;
        for (const auto& engine : per_engine)
            busiest = engine.second > busiest ? engine.second : busiest;
        return Clamp100(busiest);
    }

    PDH_HQUERY   query_ = nullptr;
    PDH_HCOUNTER cpu_ = nullptr;
    PDH_HCOUNTER gpu_ = nullptr;
    FILETIME     prev_idle_ = {}, prev_kernel_ = {}, prev_user_ = {};
};

#elif defined(__linux__)

// /proc for CPU and RAM; GPU load from the kernel driver when it exposes one (AMD / some Intel).
class Sampler {
public:
    Sampler() {
        ReadCpu(prev_idle_, prev_total_);
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm", ec)) {
            const auto path = entry.path() / "device" / "gpu_busy_percent";
            if (std::filesystem::exists(path, ec)) {
                gpu_path_ = path.string();
                break;
            }
        }
    }

    SystemMonitor::Stats Sample() {
        SystemMonitor::Stats s;
        unsigned long long idle = 0, total = 0;
        if (ReadCpu(idle, total) && total > prev_total_) {
            const double d_total = static_cast<double>(total - prev_total_);
            s.cpu_percent = Clamp100(100.0 * (1.0 - static_cast<double>(idle - prev_idle_) / d_total));
        }
        prev_idle_ = idle;
        prev_total_ = total;

        std::ifstream meminfo("/proc/meminfo");
        std::string key;
        double value = 0, total_kb = -1, avail_kb = -1;
        std::string unit;
        while (meminfo >> key >> value) {
            std::getline(meminfo, unit);
            if (key == "MemTotal:")
                total_kb = value;
            else if (key == "MemAvailable:")
                avail_kb = value;
        }
        if (total_kb > 0 && avail_kb >= 0) {
            s.ram_total_gb = static_cast<float>(total_kb / (1024.0 * 1024.0));
            s.ram_used_gb = static_cast<float>((total_kb - avail_kb) / (1024.0 * 1024.0));
        }

        if (!gpu_path_.empty()) {
            std::ifstream gpu(gpu_path_);
            int busy = -1;
            if (gpu >> busy && busy >= 0)
                s.gpu_percent = Clamp100(busy);
        }
        return s;
    }

private:
    static bool ReadCpu(unsigned long long& idle, unsigned long long& total) {
        std::ifstream stat("/proc/stat");
        std::string cpu;
        unsigned long long v[8] = {};
        if (!(stat >> cpu) || cpu != "cpu")
            return false;
        for (unsigned long long& x : v)
            stat >> x;
        idle = v[3] + v[4];  // idle + iowait
        total = 0;
        for (unsigned long long x : v)
            total += x;
        return true;
    }

    unsigned long long prev_idle_ = 0, prev_total_ = 0;
    std::string        gpu_path_;
};

#else

class Sampler {
public:
    SystemMonitor::Stats Sample() { return {}; }
};

#endif

} // namespace

SystemMonitor::~SystemMonitor() { Stop(); }

void SystemMonitor::Start() {
    if (IsRunning())
        return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = false;
    }
    thread_ = std::thread(&SystemMonitor::Run, this);
}

void SystemMonitor::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    wake_.notify_all();
    if (thread_.joinable())
        thread_.join();
}

SystemMonitor::Stats SystemMonitor::Get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void SystemMonitor::Run() {
    Sampler sampler;
    std::unique_lock<std::mutex> lock(mutex_);
    // First reading after a short delay (usage is measured between two samples), then once per second.
    auto delay = std::chrono::milliseconds(300);
    while (!wake_.wait_for(lock, delay, [this] { return stop_; })) {
        lock.unlock();
        const Stats stats = sampler.Sample();
        lock.lock();
        stats_ = stats;
        delay = std::chrono::milliseconds(1000);
    }
}

} // namespace foxy
