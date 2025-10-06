@echo off
echo ========================================
echo   Vibe3D Vulkan Shader Compiler
echo ========================================
echo.

cd /d "%~dp0shaders"

echo Compiling vertex shaders...
glslc cube.vert -o cube.vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile cube.vert
    pause
    exit /b 1
)
echo [OK] cube.vert.spv

glslc shadow_cascade.vert -o shadow_cascade.vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile shadow_cascade.vert
    pause
    exit /b 1
)
echo [OK] shadow_cascade.vert.spv

echo.
echo Compiling fragment shaders...
glslc cube.frag -o cube.frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile cube.frag
    pause
    exit /b 1
)
echo [OK] cube.frag.spv

glslc shadow_cascade.frag -o shadow_cascade.frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile shadow_cascade.frag
    pause
    exit /b 1
)
echo [OK] shadow_cascade.frag.spv

echo.
echo ========================================
echo   All shaders compiled successfully!
echo ========================================
echo.
echo Shader files created:
dir /b *.spv
echo.
pause
