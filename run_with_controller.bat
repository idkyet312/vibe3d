@echo off
REM Quick start script for Shadow Bias Controller

echo ===================================================
echo  Vibe3D Shadow Bias Controller - Quick Start
echo ===================================================
echo.

REM Check if PyQt6 is installed
py -c "import PyQt6" 2>nul
if errorlevel 1 (
    echo [!] PyQt6 not found. Installing...
    py -m pip install PyQt6
    echo.
)

echo [*] Starting Shadow Bias Controller GUI...
start py shadow_bias_controller.py

timeout /t 2 /nobreak >nul

echo [*] Starting Vibe3D...
echo.
echo You can now adjust shadow bias parameters in real-time!
echo - Use the GUI sliders to change values
echo - Changes apply immediately to the running application
echo.

.\build\vibe3d.exe

echo.
echo Application closed.
pause
