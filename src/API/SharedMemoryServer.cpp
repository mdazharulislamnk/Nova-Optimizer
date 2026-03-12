#include "API/SharedMemoryServer.hpp"
#include <iostream>

namespace nova {
namespace api {

SharedMemoryServer::SharedMemoryServer(const std::wstring& region_name) 
    : region_name_(region_name) {}

SharedMemoryServer::~SharedMemoryServer() {
    if (mapped_data_) {
        UnmapViewOfFile(mapped_data_);
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
    }
}

bool SharedMemoryServer::initialize() {
    if (initialized_) return true;

    // Create a named file mapping object using system paging file
    mapping_handle_ = CreateFileMappingW(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        sizeof(SystemPulseData), // maximum object size (low-order DWORD)
        region_name_.c_str());   // name of mapping object

    if (mapping_handle_ == nullptr) {
        std::wcerr << L"Could not create file mapping object (" << GetLastError() << L").\n";
        return false;
    }

    // Map the view of the file into our process's address space
    mapped_data_ = static_cast<SystemPulseData*>(MapViewOfFile(
        mapping_handle_,         // handle to map object
        FILE_MAP_ALL_ACCESS,     // read/write permission
        0,
        0,
        sizeof(SystemPulseData)));

    if (mapped_data_ == nullptr) {
        std::wcerr << L"Could not map view of file (" << GetLastError() << L").\n";
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        return false;
    }

    // Initialize sequence lock if we created the memory
    // If it already existed, we still just overwrite, we are the producer.
    mapped_data_->sequence.store(0, std::memory_order_relaxed);
    mapped_data_->cpu_usage_percent = 0.0;
    mapped_data_->total_ram_bytes = 0;
    mapped_data_->used_ram_bytes = 0;
    mapped_data_->ram_usage_percent = 0.0;

    initialized_ = true;
    return true;
}

void SharedMemoryServer::update_pulse(double cpu, DWORDLONG total_ram, DWORDLONG used_ram, double ram_percent) {
    if (!initialized_ || !mapped_data_) return;

    // Sequence Lock Writing Pattern
    // 1. Increment sequence to odd (signifies write in progress)
    uint64_t seq = mapped_data_->sequence.load(std::memory_order_relaxed);
    mapped_data_->sequence.store(seq + 1, std::memory_order_release);

    // 2. Write data payload
    mapped_data_->cpu_usage_percent = cpu;
    mapped_data_->total_ram_bytes = total_ram;
    mapped_data_->used_ram_bytes = used_ram;
    mapped_data_->ram_usage_percent = ram_percent;

    // 3. Increment sequence to even (signifies write complete)
    // Release memory order ensures the payload writes are visible before the sequence increments
    mapped_data_->sequence.store(seq + 2, std::memory_order_release);
}

} // namespace api
} // namespace nova
