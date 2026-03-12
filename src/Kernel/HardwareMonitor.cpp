#include "Kernel/HardwareMonitor.hpp"
#include <iostream>

namespace nova {
namespace kernel {

HardwareMonitor::HardwareMonitor() = default;

HardwareMonitor::~HardwareMonitor() {
    if (cpu_query_ != nullptr) {
        PdhCloseQuery(cpu_query_);
    }
}

bool HardwareMonitor::initialize() {
    if (initialized_) return true;

    // Open the PDH Query
    PDH_STATUS status = PdhOpenQuery(nullptr, NULL, &cpu_query_);
    if (status != ERROR_SUCCESS) {
        std::cerr << "Failed to open PDH Query. Error: 0x" << std::hex << status << std::endl;
        return false;
    }

    // Add counter for total CPU Time
    // Note: The counter path might be localized. Using generic counter path format.
    const char* counter_path = "\\Processor(_Total)\\% Processor Time";
    status = PdhAddCounterA(cpu_query_, counter_path, NULL, &cpu_total_counter_);
    if (status != ERROR_SUCCESS) {
        std::cerr << "Failed to add PDH Counter. Error: 0x" << std::hex << status << std::endl;
        PdhCloseQuery(cpu_query_);
        cpu_query_ = nullptr;
        return false;
    }

    // Perform an initial poll to populate values
    PdhCollectQueryData(cpu_query_);

    // Get Total RAM capacity once (assuming it doesn't change during runtime)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        total_ram_bytes_ = memInfo.ullTotalPhys;
    }

    initialized_ = true;
    return true;
}

void HardwareMonitor::poll() {
    if (!initialized_) return;

    // CPU Polling
    PDH_STATUS status = PdhCollectQueryData(cpu_query_);
    if (status == ERROR_SUCCESS) {
        PDH_FMT_COUNTERVALUE counter_val;
        status = PdhGetFormattedCounterValue(cpu_total_counter_, PDH_FMT_DOUBLE, nullptr, &counter_val);
        if (status == ERROR_SUCCESS) {
            current_cpu_usage_ = counter_val.doubleValue;
        }
    }

    // RAM Polling
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        used_ram_bytes_ = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
        current_ram_percent_ = static_cast<double>(memInfo.dwMemoryLoad);
    }
}

double HardwareMonitor::get_cpu_usage() const {
    return current_cpu_usage_;
}

DWORDLONG HardwareMonitor::get_total_ram_bytes() const {
    return total_ram_bytes_;
}

DWORDLONG HardwareMonitor::get_used_ram_bytes() const {
    return used_ram_bytes_;
}

double HardwareMonitor::get_ram_usage_percent() const {
    return current_ram_percent_;
}

} // namespace kernel
} // namespace nova
