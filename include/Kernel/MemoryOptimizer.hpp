#pragma once

#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace nova {
namespace kernel {

/**
 * MemoryOptimizer provides tools for optimizing system memory, such as trimming
 * working sets for idle processes.
 */
class MemoryOptimizer {
public:
    MemoryOptimizer() = default;
    ~MemoryOptimizer() = default;

    /**
     * Aggressively empties the working set of a target process, sending idle
     * memory pages to the page file to free up physical RAM.
     * Use with caution.
     */
    static bool trim_process_working_set(DWORD process_id);

    /**
     * Attempts to trim the working set of all accessible processes on the system.
     * Useful for performing a global memory defragmentation/optimization pulse.
     */
    static size_t optimize_all_processes();
};

} // namespace kernel
} // namespace nova
