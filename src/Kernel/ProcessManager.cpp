#include "Kernel/ProcessManager.hpp"
#include <tlhelp32.h>
#include <iostream>

namespace nova {
namespace kernel {

std::vector<std::pair<DWORD, std::wstring>> ProcessManager::get_running_processes() {
    std::vector<std::pair<DWORD, std::wstring>> processes;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &process_entry)) {
        do {
            processes.emplace_back(process_entry.th32ProcessID, std::wstring(process_entry.szExeFile));
        } while (Process32NextW(snapshot, &process_entry));
    }

    CloseHandle(snapshot);
    return processes;
}

bool ProcessManager::set_process_priority(DWORD process_id, DWORD priority_class) {
    HANDLE process_handle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, process_id);
    if (!process_handle) {
        return false;
    }

    BOOL result = SetPriorityClass(process_handle, priority_class);
    CloseHandle(process_handle);
    return result != 0;
}

bool ProcessManager::set_process_affinity(DWORD process_id, DWORD_PTR affinity_mask) {
    HANDLE process_handle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, process_id);
    if (!process_handle) {
        return false;
    }

    BOOL result = SetProcessAffinityMask(process_handle, affinity_mask);
    CloseHandle(process_handle);
    return result != 0;
}

bool ProcessManager::elevate_current_process() {
    HANDLE current_process = GetCurrentProcess();
    BOOL result = SetPriorityClass(current_process, REALTIME_PRIORITY_CLASS);
    return result != 0;
}

} // namespace kernel
} // namespace nova
