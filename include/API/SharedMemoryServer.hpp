#pragma once

#include <windows.h>
#include <string>
#include <atomic>

namespace nova {
namespace api {

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignas
/**
 * Data structure exposed via Shared Memory.
 * Uses a sequence lock pattern for lock-free multi-reader consistency.
 */
struct alignas(64) SystemPulseData {
    std::atomic<uint64_t> sequence{0}; // Incremented before and after writes
    
    double cpu_usage_percent{0.0};
    DWORDLONG total_ram_bytes{0};
    DWORDLONG used_ram_bytes{0};
    double ram_usage_percent{0.0};
    
    // Padding to 64 bytes for cache line alignment if needed, though alignas handles the struct start.
};
#pragma warning(pop)

/**
 * SharedMemoryServer sets up a named memory-mapped file for IPC.
 * It is responsible for writing the latest system metrics so client applications
 * can read them with near-zero latency.
 */
class SharedMemoryServer {
public:
    explicit SharedMemoryServer(const std::wstring& region_name);
    ~SharedMemoryServer();

    SharedMemoryServer(const SharedMemoryServer&) = delete;
    SharedMemoryServer& operator=(const SharedMemoryServer&) = delete;

    /**
     * Initializes the shared memory segment.
     * Returns true on success.
     */
    bool initialize();

    /**
     * Updates the shared memory with new performance data.
     * Uses a sequence lock to ensure readers don't read torn data.
     */
    void update_pulse(double cpu, DWORDLONG total_ram, DWORDLONG used_ram, double ram_percent);

private:
    std::wstring region_name_;
    HANDLE mapping_handle_{nullptr};
    SystemPulseData* mapped_data_{nullptr};
    bool initialized_{false};
};

} // namespace api
} // namespace nova
