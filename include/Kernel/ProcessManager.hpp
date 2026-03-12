#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace nova {
namespace kernel {

/**
 * ProcessManager allows for priority and CPU affinity manipulation.
 */
class ProcessManager {
public:
    ProcessManager() = default;
    ~ProcessManager() = default;

    /**
     * Gets a list of currently running processes with their IDs and Names.
     */
    static std::vector<std::pair<DWORD, std::wstring>> get_running_processes();

    /**
     * Sets the priority class of a specific process.
     * Common priorities: IDLE_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS,
     * HIGH_PRIORITY_CLASS, REALTIME_PRIORITY_CLASS.
     * Returns true on success.
     */
    static bool set_process_priority(DWORD process_id, DWORD priority_class);

    /**
     * Sets the CPU affinity mask for a specific process.
     * A bitmask where each bit represents a CPU core (e.g., 0x3 for cores 0 and 1).
     * Returns true on success.
     */
    static bool set_process_affinity(DWORD process_id, DWORD_PTR affinity_mask);

    /**
     * Elevates the current process to REALTIME_PRIORITY_CLASS for critical polling.
     */
    static bool elevate_current_process();
};

} // namespace kernel
} // namespace nova
