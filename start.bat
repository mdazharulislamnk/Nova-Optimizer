@echo off
setlocal
echo ==================================================
echo   Starting Nova-Optimizer Unified Environment
echo ==================================================
echo.

:: 1. Check if the project is built, if not, build it.
if not exist "build\Release\Nova_Optimizer.exe" (
    echo [Build] Nova_Optimizer.exe not found. Building project...
    cmake -B build
    cmake --build build --config Release
    
    if %errorlevel% neq 0 (
        echo [Error] Build failed! Check cmake output.
        pause
        exit /b 1
    )
    echo [Build] Success!
)

:: 2. Start the Nova Optimization Engine in the background
echo [Launcher] Starting C++ Nova-Optimizer Engine...
start "Nova Engine (C++)" cmd /c ".\build\Release\Nova_Optimizer.exe"

:: 3. Give the engine a second to allocate the shared memory
timeout /t 1 /nobreak > nul

:: 4. Start the Python Dashboard Server
echo [Launcher] Starting Python API Bridge and Dashboard Server...
cd dashboard
start "Nova Dashboard Server (Python)" cmd /c "python server.py"
cd ..

:: 5. Open the Web Dashboard in the default browser
echo [Launcher] Opening Dashboard in your browser...
start http://localhost:8080/dashboard.html

echo.
echo ==================================================
echo   All systems running. 
echo   Close the popped-up command windows to stop perfectly.
echo   Press any key to exit this launcher window.
echo ==================================================
pause > nul
