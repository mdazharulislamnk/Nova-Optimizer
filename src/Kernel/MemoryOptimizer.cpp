#include "Kernel/MemoryOptimizer.hpp"
#include "Kernel/ProcessManager.hpp"
#include <iostream>

namespace nova {
namespace kernel {

bool MemoryOptimizer::trim_process_working_set(DWORD process_id) {
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA | PROCESS_VM_READ, FALSE, process_id);
    
    if (!process_handle) {
        return false; // Access Denied or process no longer valid
    }

    // Windows API allows us to try and shrink the working set to minimum memory required
    // Passing (SIZE_T)-1 forces the OS to empty the working set as much as possible to the page file
    BOOL result = EmptyWorkingSet(process_handle);
    
    // Alternatively, one could use SetProcessWorkingSetSize(process_handle, (SIZE_T)-1, (SIZE_T)-1);
    
    CloseHandle(process_handle);
    return result != 0;
}

size_t MemoryOptimizer::optimize_all_processes() {
    size_t optimized_count = 0;
    auto processes = ProcessManager::get_running_processes();

    // Skip our own process to avoid trashing our performance
    DWORD current_pid = GetCurrentProcessId();

    for (const auto& [pid, name] : processes) {
        if (pid == 0 || pid == current_pid) continue; // Skip Idle process and ourselves

        if (trim_process_working_set(pid)) {
            optimized_count++;
        }
    }

    return optimized_count;
}

} // namespace kernel
} // namespace nova
