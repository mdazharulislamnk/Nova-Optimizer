#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "Kernel/HardwareMonitor.hpp"
#include "Kernel/ProcessManager.hpp"
#include "Kernel/MemoryOptimizer.hpp"
#include "API/SharedMemoryServer.hpp"
#include "Utils/LockFreeBuffer.hpp"

using namespace nova;

// Global flag to handle graceful shutdown
std::atomic<bool> g_running{true};

// Handle Ctrl+C for clean exit
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        std::cout << "\n[Nova-Optimizer] Shutting down gracefully...\n";
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

/**
 * The Data payload passed between threads via LockFreeBuffer.
 */
struct InternalPulseData {
    double cpu_usage;
    DWORDLONG total_ram;
    DWORDLONG used_ram;
    double ram_percent;
};

// Thread-safe lock-free buffer with capacity of 1024 frames (must be power of 2)
nova::utils::LockFreeBuffer<InternalPulseData, 1024> g_pulse_buffer;

void hardware_polling_thread() {
    std::cout << "[Hardware Thread] Starting high-res polling (10ms)...\n";

    kernel::HardwareMonitor monitor;
    if (!monitor.initialize()) {
        std::cerr << "[Hardware Thread] Failed to initialize hardware monitor.\n";
        return;
    }

    // 10ms high-resolution timer
    const auto poll_interval = std::chrono::milliseconds(10);
    // Periodically run memory optimization every 10 seconds
    const int optimization_ticks = 1000; 
    int current_tick = 0;

    while (g_running.load(std::memory_order_relaxed)) {
        auto start_time = std::chrono::steady_clock::now();

        // 1. Poll Sensors
        monitor.poll();

        // 2. Prepare Data Frame
        InternalPulseData data;
        data.cpu_usage = monitor.get_cpu_usage();
        data.total_ram = monitor.get_total_ram_bytes();
        data.used_ram = monitor.get_used_ram_bytes();
        data.ram_percent = monitor.get_ram_usage_percent();

        // 3. Dispatch to API Thread via Lock-Free Buffer
        if (!g_pulse_buffer.push(data)) {
            // Buffer full, potential bottleneck in API thread
            // Skip frame
        }

        // 4. Memory Optimization logic (runs every 10 seconds)
        current_tick++;
        if (current_tick >= optimization_ticks) {
            current_tick = 0;
            if (data.ram_percent > 85.0) { // Aggressively optimize if memory usage is very high
                std::cout << "[Optimizer] RAM usage critical (>85%). Trimming external processes.\n";
                // Start a detached thread to optimize, preventing blocking of the high-res polling
                std::thread([]() {
                    size_t optimized = kernel::MemoryOptimizer::optimize_all_processes();
                    std::cout << "[Optimizer] Trimmed working sets for " << optimized << " processes.\n";
                }).detach();
            }
        }

        // 5. Sleep for remainder of interval
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (elapsed < poll_interval) {
            std::this_thread::sleep_for(poll_interval - elapsed);
        }
    }
    
    std::cout << "[Hardware Thread] Stopped.\n";
}

void api_server_thread() {
    std::cout << "[API Server Thread] Starting Shared Memory IPC Server...\n";

    std::unique_ptr<api::SharedMemoryServer> shm_server = std::make_unique<api::SharedMemoryServer>(L"Global\\NovaOptimizer_Pulse");
    if (!shm_server->initialize()) {
        std::cerr << "[API Server Thread] Failed to initialize Shared Memory Server in Global namespace.\n";
        std::cout << "[API Server Thread] Retrying with Local namespace...\n";
        shm_server = std::make_unique<api::SharedMemoryServer>(L"Local\\NovaOptimizer_Pulse");
        if (!shm_server->initialize()) {
             std::cerr << "[API Server Thread] Failed to initialize Local Shared Memory Server. IPC disabled.\n";
             return;
        }
    }

    std::cout << "[API Server Thread] Shared Memory Segment mapped successfully.\n";

    while (g_running.load(std::memory_order_relaxed)) {
        // Pop events from lock-free queue
        auto data_opt = g_pulse_buffer.pop();
        if (data_opt.has_value()) {
            // Write directly to shared memory
            shm_server->update_pulse(
                data_opt->cpu_usage, 
                data_opt->total_ram, 
                data_opt->used_ram, 
                data_opt->ram_percent
            );
        } else {
            // Yield to avoid busy-waiting 100% CPU when buffer is empty
            std::this_thread::yield();
            // Fallback sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "[API Server Thread] Stopped.\n";
}

int main() {
    std::cout << "==========================================\n";
    std::cout << " Nova-Optimizer - Hardware Sync Engine\n";
    std::cout << "==========================================\n";

    // Register Console Control Handler
    SetConsoleCtrlHandler(console_handler, TRUE);

    // Elevate this process to REALTIME priority to guarantee millisecond accuracy
    // WARNING: This requires Administrator privileges. If not admin, it will default to HIGH.
    if (kernel::ProcessManager::elevate_current_process()) {
        std::cout << "[Main] Process elevated to REALTIME priority class successfully.\n";
    } else {
        std::cout << "[Main] Warning: Could not elevate process (Run as Admin for best results).\n";
        // Fallback to HIGH priority
        kernel::ProcessManager::set_process_priority(GetCurrentProcessId(), HIGH_PRIORITY_CLASS);
    }

    // Set CPU affinity for current process to Core 0 to minimize context switching overhead
    // This is optional but demonstrates advanced control. Let's just affinity bind the whole process to Core 0 (0x1) and Core 1 (0x2).
    kernel::ProcessManager::set_process_affinity(GetCurrentProcessId(), 0x3);

    // Launch worker threads
    std::thread hardware_thread(hardware_polling_thread);
    std::thread api_thread(api_server_thread);

    std::cout << "[Main] Engine running. Press Ctrl+C to stop.\n\n";

    // Main thread just waits for the threads to finish joining
    hardware_thread.join();
    api_thread.join();

    std::cout << "[Main] Engine terminated gracefully.\n";
    return 0;
}
