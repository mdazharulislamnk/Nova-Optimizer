# Nova-Optimizer
### Hardware-Synchronized System Engine & Telemetry Dashboard

An advanced, high-performance Producer-Consumer C++ engine that interacts directly with the Windows Kernel via the Win32 API. It monitors hardware at the millisecond level, utilizes a Lock-Free Ring Buffer to bypass mutex latency, and exposes a zero-latency Inter-Process Communication (IPC) memory-mapped API.

Built alongside it is a lightweight Python Bridge Server and a beautifully designed, glowing glassmorphism HTML5/JS Web Dashboard that visualizes the raw system memory in real-time.

---

## Tech Stack
- **Core Engine:** C++20, MSVC, CMake
- **Operating System Focus:** Windows Kernel API (`pdh.h`, `psapi.h`, `windows.h`)
- **Concurrency Pattern:** Single-Producer Single-Consumer (SPSC) Lock-Free Ring Buffer
- **Inter-Process Communication:** Named Shared Memory Mapping, Sequence Locks
- **Backend Bridge:** Python 3 (Native `http.server`, `mmap`, `struct`)
- **Frontend Dashboard:** HTML5, CSS3 Variables, Vanilla JavaScript, jsPDF
- **UI Design Aesthetic:** Premium Glassmorphism, CSS Conic Gradients, Google Fonts

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [Why Build This? (The Engineering Challenge)](#why-build-this-the-engineering-challenge)
3. [Project Architecture tree](#project-architecture-tree)
4. [Installation Requirements](#installation-requirements)
5. [Step-by-Step Build & Run Guide](#step-by-step-build--run-guide)
6. [Complete System Walkthrough](#complete-system-walkthrough)
7. [Detailed File Explanations & CS Concepts](#detailed-file-explanations--cs-concepts)

---

## Project Overview

Nova-Optimizer answers the challenge of building performance-critical, low-level system tooling. The goal of this engine is not just to monitor metrics, but to demonstrate advanced concurrent architecture:

1. **System Tapping**: Utilizing the Windows PDH API to perfectly count real-time CPU cycles and RAM bytes without task-manager delays.
2. **Lock-Free Sharing**: Pushing 64-byte structs across a ring-buffer without a single mutex `lock()`.
3. **Zero-Latency Serving**: Keeping an IPC Memory-Mapped file updated constantly alongside a "Sequence Lock" tick, allowing external applications (like python, or another C++ instance) to peek into the active memory safely alongside the main application.
4. **Autonomous Testing**: Running timed background tests straight from a web UI and instantly generating PDF Reports entirely from client-side JS.

---

## Why Build This? (The Engineering Challenge)

Most modern applications rely on high-level garbage-collected languages (like JavaScript or Python) to read system statistics, which inherently introduces latency, polling delays, and "heavy" footprint overhead. 

If you want to build **machine control software**, **High-Frequency Trading platforms**, or **Game Engines**, you need software that executes in microseconds, guarantees memory safety across cores, and operates without stalling.

This project was built to prove understanding of those extreme conditions:
- **How do two threads talk to each other without stopping?** We built a Lock-Free Ring buffer using `std::atomic` operations. This means the thread gathering data never has to wait in line for the thread sharing data. They run infinitely in parallel.
- **How do two completely separate programs share data instantly?** We used a Named Memory Map. Both the C++ Engine and the Python server look at the exact same physical byte block in RAM. When C++ writes to it, Python instantly sees it without needing a slow TCP/IP port connection.
- **How do we know the shared memory isn't corrupted mid-read?** We utilized a "Sequence Lock". C++ writes an odd number `1`, writes the data, then writes an even number `2`. Python only reads the data when it sees an even number, completely eliminating the need for slow Cross-Process Mutex locks!

---

## Project Architecture Tree

```plaintext
Nova_Optimizer/
├── CMakeLists.txt                 # Compilation instructions
├── start.bat                      # Unified Launcher Script
├── test_shared_memory.py          # Python CLI log script for reading the API
│
├── dashboard/
│   ├── dashboard.html             # The Premium Frontend Single-Page App
│   └── server.py                  # The Python Bridge API Server
│
├── include/
│   ├── API/
│   │   └── SharedMemoryServer.hpp # Shared Memory Mapping Core Header
│   ├── Kernel/
│   │   ├── HardwareMonitor.hpp    # Windows PDH Telemetry Hooks Header
│   │   ├── MemoryOptimizer.hpp    # Process Working-Set Trimmer Header
│   │   └── ProcessManager.hpp     # Base Process Hooks Header
│   └── Utils/
│       └── LockFreeBuffer.hpp     # Multi-thread SPSC Ring Buffer Module
│
└── src/
    ├── main.cpp                   # The Master Orchestrator Application
    ├── API/
    │   └── SharedMemoryServer.cpp # Shared Memory Implementation 
    └── Kernel/
        ├── HardwareMonitor.cpp    # CPU/RAM fetching logic
        ├── MemoryOptimizer.cpp    # Memory footprint trimming logic
        └── ProcessManager.cpp     # Execution Priority modification
```

---

## Installation Requirements

This project utilizes specific core libraries built into the Windows Native system.

- **OS:** Windows 10 / 11 
- **Compiler:** MSVC (Requires Visual Studio with "Desktop development with C++")
- **Build System:** CMake (Version 3.10+)
- **Scripting:** Python 3.8+ (Added to system PATH)

---

## Step-by-Step Build & Run Guide

To make the developer experience seamless, this project comes with an orchestrated launch script that builds the C++ code, launches the background processes, boots standard web servers, and navigates your browser.

1. Ensure CMake and Python are installed.
2. Open a Terminal / Command Prompt in the `Nova_Optimizer` folder.
3. Simply execute the Batch script:
   ```cmd
   start.bat
   ```

**What the script does automatically:**
1. Checks if `build/Release/Nova_Optimizer.exe` exists.
2. If it does not, it commands `cmake` to elegantly compile the 11 C++ files together.
3. Once built, it spawns `Nova_Optimizer.exe` as an invisible background process.
4. It navigates to the `dashboard/` directory and spins up `python server.py`.
5. Finally, it pops open your default browser to `http://localhost:8080/dashboard.html`.

### Stopping the System
When you are done testing, simply close out the two spawned command-line boxes (The C++ Engine Box and the Python Server Box) and they will gracefully cleanly terminate the entire memory architecture.

---

## Complete System Walkthrough

Once you trigger `start.bat` and the Dashboard UI appears in your browser, here's how to use the specific functionalities:

### 1. Visualizing the Memory Hooks
The moment you load the dashboard, you will notice the Dial-Meters for CPU and RAM updating fiercely. The JavaScript frontend is querying the Python Server every `100` milliseconds, giving the entire display a rapid, "0-latency" feel. The Python Server is in turn instantly mapping the C++ Shared memory.

The numbers you are seeing are the actual exact physical byte loads on your machine right now.

### 2. Manual Start & Stop
In the top right corner, there are Control Buttons. 
- You can press **⬛ STOP** to cleanly execute a `taskkill` signal over the backend API and shut down the background `Nova_Optimizer.exe`.
- When stopped, the dashboard dials safely slide back to `0` and a `STANDBY` state gracefully activates.
- Clicking **▶ START ENGINE** triggers an asynchronous `subprocess.Popen` task in python to securely boot the engine back up, turning the meters back online.

### 3. Deep Kernel Details
Click the **"View Advanced Metrics"** button in the lower left IPC section. A frosted glass window overlays the screen. 

Look closely at the `Sequence Lock (Tick)` metric updating rapidly. This confirms everything the project aims to do: that number ticks up continuously on the C++ side without ever using standard locking mechanisms.

### 4. Running a Timed System Test
In the top right corner under the start controls:
- Place a minute denomination in the box (e.g. `0.2` for 12 seconds).
- Click **⚡ RUN TEST**.
This activates python's `test_shared_memory.py` in the background with a specific timer. Click the newly materialized modal window and watch the physical memory logs stream into that box cleanly.

### 5. Final PDF Exporting
Once a test has run, you can hit the **"📄 EXPORT TEST LOGS PDF"** button inside the modal (Or the main Export Dashboard PDF button). A beautiful, multi-page layout is instantaneously compiled using Javascript and downloaded to your hard drive to send off for data analysis, detailing everything from the sequence ticks to the Exact RAM string outputs.

---

## Detailed File Explanations & CS Concepts

Here is a full breakdown of the inner architecture so anyone can understand what the files are doing.

#### `CMakeLists.txt`
Instructions for the compiler. Targets C++20 and explicitly commands MSVC to link the special `pdh.lib` (Performance Counters) and `psapi.lib` (Process Info) windows libraries needed to grab the core metrics.

#### `LockFreeBuffer.hpp`
A high-performance circular Ring Buffer utilizing `std::atomic` values. 
**Computer Science Concept:** Standard multithreading uses `std::mutex` which literally commands a thread to "Go to sleep" while another thread reads data. A Lock-Free buffer uses explicit `std::memory_order_release` and `std::memory_order_acquire`. This guarantees that The "Producer" Thread (pushing data) and the "Consumer" Thread (reading data) can safely overwrite and read memory pointers without ever halting execution of their while-loops. 

#### `HardwareMonitor.cpp` / `.hpp`
The brain of the Kernel sensors. Initializes a query (`PdhOpenQuery`) and hooks counters (`PdhAddEnglishCounterW`) for `\Processor(_Total)\% Processor Time`. It acts far quicker and with far less footprint than WMI queries or standard tasklist executions.

#### `MemoryOptimizer.cpp` / `ProcessManager.cpp`
The active-control mechanics. While the app reads memory, it also modifies the engine instance using `SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)`. This ensures when the system is heavily burdened, Nova-Optimizer is never pushed down the CPU scheduler stack.

#### `SharedMemoryServer.cpp`
The core IPC mechanic file. Calls `CreateFileMapping` to create a named region (`Local\NovaOptimizer_Pulse`). 
**Computer Science Concept (Sequence Locks):** 
When two programs share memory, if one program reads data exactly as the other writes it, you get garbage "Torn Bytes". This file uses a sequence lock:
1. It updates an atomic number: `1` (Odd == Currently Writing).
2. It writes the memory bitstruct out over the 64 bytes.
3. It updates the atomic number: `2` (Even == Finished Writing).
On the python side, it only processes the memory if it grabs an even number before and after looking at it!

#### `main.cpp`
The maestro. It starts the multi-threading instances side-by-side using `std::thread`, hooking `HardwareMonitor` arrays into the `SharedMemoryServer` via the `LockFreeBuffer` pipe safely. It maintains the master polling loop using high-precision sleep (`std::this_thread::sleep_for(10ms)`).

#### `dashboard/server.py`
A python script running a generic `SimpleHTTPRequestHandler`. On a GET request to `/api/pulse`, it uses `mmap` to open `Local\NovaOptimizer_Pulse` and slices the 64 raw bytes out of the kernel into python integers using Python's Native C-Struct library (`struct.unpack()`). It acts as a JSON bridge out to the Web UI. It also processes POST requests using python's `subprocess` to control background binaries.

#### `dashboard/dashboard.html`
A single-file HTML element injected perfectly with embedded sleek custom CSS definitions and Vanilla Javascript. Uses modern responsive DOM grids, keyframed pulse animations, and CSS conic gradients to draw dynamic visuals. Integrates `jsPDF` for client-end report generation.
