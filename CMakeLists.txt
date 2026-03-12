cmake_minimum_required(VERSION 3.20)
project(Nova_Optimizer VERSION 1.0.0 LANGUAGES CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add Include Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define source files
set(SOURCES
    src/main.cpp
    src/Kernel/HardwareMonitor.cpp
    src/Kernel/ProcessManager.cpp
    src/Kernel/MemoryOptimizer.cpp
    src/API/SharedMemoryServer.cpp
)

# Add Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link necessary Windows libraries
# pdh.lib - Performance Data Helper for hardware monitoring
# psapi.lib - Process Status API for memory management
target_link_libraries(${PROJECT_NAME} PRIVATE pdh psapi)

# Enable compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif()
