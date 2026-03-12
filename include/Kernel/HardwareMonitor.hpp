#pragma once

#include <windows.h>
#include <pdh.h>
#include <string>

#pragma comment(lib, "pdh.lib")

namespace nova {
namespace kernel {

/**
 * HardwareMonitor utilizes the Performance Data Helper (PDH) API to 
 * poll CPU and RAM metrics at high resolution.
 */
class HardwareMonitor {
public:
    HardwareMonitor();
    ~HardwareMonitor();

    // Prevent copying
    HardwareMonitor(const HardwareMonitor&) = delete;
    HardwareMonitor& operator=(const HardwareMonitor&) = delete;

    /**
     * Initializes the PDH query and adds required counters.
     * Returns true on success, false otherwise.
     */
    bool initialize();

    /**
     * Polls the latest hardware metrics from the initialized counters.
     * Should be called periodically (e.g., every 10-100ms).
     */
    void poll();

    /**
     * Gets the total CPU usage as a percentage (0.0 to 100.0).
     */
    double get_cpu_usage() const;

    /**
     * Gets the total Physical RAM in bytes.
     */
    DWORDLONG get_total_ram_bytes() const;

    /**
     * Gets the currently used Physical RAM in bytes.
     */
    DWORDLONG get_used_ram_bytes() const;

    /**
     * Gets the RAM usage as a percentage (0.0 to 100.0).
     */
    double get_ram_usage_percent() const;

private:
    PDH_HQUERY cpu_query_{nullptr};
    PDH_HCOUNTER cpu_total_counter_{nullptr};
    
    double current_cpu_usage_{0.0};
    DWORDLONG total_ram_bytes_{0};
    DWORDLONG used_ram_bytes_{0};
    double current_ram_percent_{0.0};
    
    bool initialized_{false};
};

} // namespace kernel
} // namespace nova
