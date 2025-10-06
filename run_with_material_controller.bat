@echo off
echo Starting Vibe3D Material Controller...
start "Material Controller" py material_controller.py

timeout /t 1 /nobreak > nul

echo Starting Vibe3D Application...
cd build
start "Vibe3D" vibe3d.exe
cd ..

echo.
echo Both Material Controller and Vibe3D are now running!
echo Close this window if you want to keep them running.
echo.
