# Vibe3D Setup Instructions

## Prerequisites

- Visual Studio 2022 with C++ development tools
- CMake (3.10 or later)
- vcpkg package manager

## Step 1: Install Dependencies with vcpkg

The project now uses vcpkg manifest mode. Dependencies should install automatically when you build, but you can also install them manually:

```bash
# If vcpkg is not in your PATH, navigate to your vcpkg installation
# cd C:\vcpkg

# Install dependencies (this happens automatically with manifest mode)
vcpkg install glfw3:x64-windows glm:x64-windows glad:x64-windows
```

## Step 2: Download and Install PhysX

### Option A: Download Precompiled PhysX (Recommended)

1. Download PhysX SDK from NVIDIA:
   - Go to https://github.com/NVIDIA-Omniverse/PhysX/releases
   - Download the latest Windows release (e.g., `PhysX-107.1-physx-5.6.1-windows.zip`)

2. Extract PhysX to `C:\PhysX\` so that the structure looks like:
   ```
   C:\PhysX\
   ??? physx\
       ??? include\
       ?   ??? PxPhysicsAPI.h
       ?   ??? ...
       ??? bin\
           ??? win.x86_64.vc143.md\
               ??? debug\
               ?   ??? PhysX_64.lib
               ?   ??? PhysXCommon_64.lib
               ?   ??? ...
               ??? release\
                   ??? PhysX_64.lib
                   ??? PhysXCommon_64.lib
                   ??? ...
   ```

### Option B: Build PhysX from Source

1. Clone PhysX repository:
   ```bash
   git clone https://github.com/NVIDIA-Omniverse/PhysX.git
   cd PhysX/physx
   ```

2. Generate Visual Studio project files:
   ```bash
   generate_projects.bat vc16win64
   ```

3. Open `PhysX/physx/compiler/vc16win64/PhysXSDK.sln` in Visual Studio

4. Build both Debug and Release configurations for x64

5. Copy the built libraries to `C:\PhysX\physx\`

## Step 3: Build the Project

```bash
# Navigate to your project directory
cd C:\Users\Bas\Source\Repos\vibe3d

# Configure with vcpkg toolchain (replace [vcpkg-root] with your vcpkg path)
cmake -S . -B out -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Example with common vcpkg location:
# cmake -S . -B out -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build out --config Release
```

## Step 4: Run the Application

```bash
# Navigate to build directory
cd out

# Run the executable
.\Release\vibe3d.exe
```

## Troubleshooting

### PhysX Issues

- **PhysX not found**: Make sure PhysX is installed at `C:\PhysX\physx\` or specify custom path:
  ```bash
  cmake -S . -B out -DPHYSX_ROOT_DIR=C:/YourPhysXPath/physx -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
  ```

- **Library mismatch**: Ensure PhysX libraries are built for the same architecture (x64) and Visual Studio version (2022/vc143)

- **Runtime errors**: Make sure PhysX DLLs are in your PATH or copy them to your executable directory

### vcpkg Issues

- **vcpkg not found**: Make sure vcpkg is properly integrated with Visual Studio:
  ```bash
  vcpkg integrate install
  ```

- **Dependencies not found**: Verify vcpkg.json is in your project root and contains the correct baseline

### Build Issues

- **OpenGL errors**: Make sure you have proper graphics drivers installed
- **Shader files missing**: The build process should automatically copy shader files to the output directory

## Controls

- **WASD**: Move camera
- **Mouse**: Look around
- **Space**: Jump
- **Left Click**: Shoot bullets
- **E**: Spawn cubes
- **ESC**: Exit

## Project Structure

```
vibe3d/
??? include/           # Header files
??? src/              # Source files
?   ??? glad.c        # OpenGL loader
??? out/              # Build output
??? test.cpp          # Main game file
??? *.glsl            # Shader files
??? CMakeLists.txt    # Build configuration
??? vcpkg.json        # Dependency manifest
??? SETUP.md          # This file