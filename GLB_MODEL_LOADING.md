# GLB Model Loading

## Overview
Vibe3D now supports loading 3D models from GLB/glTF 2.0 files. The model will replace the default cube and rotate smoothly on the yaw axis (Y-axis) only.

## Features
- **GLB/glTF 2.0 Support**: Load binary glTF files exported from Blender, Maya, or other 3D software
- **Automatic Vertex Data**: Extracts positions, normals, UVs, and vertex colors
- **Yaw-Only Rotation**: Model rotates smoothly around the Y-axis only (no pitch/roll)
- **Floor Plane**: Automatically adds a floor plane below the model
- **Fallback to Cube**: If no model file is found, uses the default colorful cube

## How to Use

### 1. Export a Model from Blender
1. Open Blender and create/load your model
2. Select your object(s)
3. Go to `File` → `Export` → `glTF 2.0 (.glb/.gltf)`
4. **Export Settings**:
   - Format: `glTF Binary (.glb)`
   - Include: Check `Selected Objects`
   - Transform: Check `+Y Up` (Vulkan coordinate system)
   - Geometry: Check `UVs`, `Normals`, `Vertex Colors` (if needed)
5. Save as `model.glb`

### 2. Place the Model File
- Copy `model.glb` to the root directory of Vibe3D (next to `vibe3d.exe`)
- Or copy it to the build directory: `vibe3d/build/model.glb`

### 3. Run the Application
```batch
cd build
vibe3d.exe
```

The application will automatically:
- Look for `model.glb` in the current directory
- Load and display the model with yaw-only rotation
- Fall back to the default cube if `model.glb` is not found

## Building with GLB Support

### Requirements
- tinygltf library (automatically installed via vcpkg)
- C++20 compiler
- CMake 3.15+

### Build Commands

#### Option 1: Visual Studio Developer Command Prompt
```batch
cd build
cmake ..
ninja
```

#### Option 2: Visual Studio IDE
1. Open the project folder in Visual Studio 2022
2. Let CMake configure automatically
3. Build → Build All

#### Option 3: VS Code with CMake Tools
1. Open VS Code in the project folder
2. Press `Ctrl+Shift+P`
3. Run "CMake: Configure"
4. Run "CMake: Build"

## Model Requirements

### Supported Features
- ✅ Multiple meshes
- ✅ Vertex positions (required)
- ✅ Vertex normals (optional, defaults to up vector)
- ✅ Texture coordinates (optional, defaults to 0,0)
- ✅ Vertex colors (optional, defaults to gray)
- ✅ Multiple primitives per mesh
- ✅ Different index types (uint8, uint16, uint32)

### Not Yet Supported
- ❌ Textures (uses vertex colors only)
- ❌ PBR materials (uses default material)
- ❌ Animations
- ❌ Skinning/rigging
- ❌ Multiple scenes

## Technical Details

### Coordinate System
- **Blender**: Z-up, Y-forward
- **Vulkan/Vibe3D**: Y-up, Z-forward
- **Export Setting**: Use "+Y Up" when exporting from Blender

### Rotation
- Yaw axis: Y-axis (vertical)
- Speed: 30 degrees per second
- Center: Model origin (0, 0.5, 0) - hovering above floor

### Vertex Structure
```cpp
struct Vertex {
    glm::vec3 position;   // X, Y, Z coordinates
    glm::vec3 normal;     // Surface normal
    glm::vec2 texCoord;   // UV coordinates (unused in current shader)
    glm::vec3 color;      // RGB vertex color
};
```

## Example Blender Workflow

1. **Create a simple character/object**:
   - Model your object
   - Apply vertex colors (optional)
   - Scale appropriately (1 Blender unit ≈ 1 meter)

2. **Prepare for export**:
   - Origin to geometry: `Object` → `Set Origin` → `Origin to Geometry`
   - Apply transforms: `Object` → `Apply` → `All Transforms`
   - Triangulate faces: `Modifier` → `Triangulate`

3. **Export**:
   - `File` → `Export` → `glTF 2.0`
   - Format: `glTF Binary (.glb)`
   - Transform: `+Y Up`
   - Geometry: All enabled

4. **Test**:
   - Place `model.glb` next to `vibe3d.exe`
   - Run the application
   - The model should appear, rotating slowly on the Y-axis

## Troubleshooting

### Model doesn't appear
- Check console output for "Loading model from: model.glb"
- Verify file is named exactly `model.glb`
- Check file is in the correct directory (same as executable)

### Model appears black
- The shader uses vertex colors - ensure your model has vertex colors
- Or the model will use default gray color (0.8, 0.8, 0.8)

### Model is wrong size
- Scale in Blender before export (Edit Mode: `S` → scale)
- 1 Blender unit = 1 meter in Vibe3D

### Model is rotated wrong
- Export with "+Y Up" setting
- Check Blender object rotation is applied (Ctrl+A → All Transforms)

### Model rotates on wrong axis
- This is intentional! Only yaw (Y-axis) rotation is used for realistic presentation
- Models rotate around their origin point

## Future Enhancements
- [ ] Texture support
- [ ] PBR material support
- [ ] Animation playback
- [ ] Multiple model support
- [ ] UI file picker for model selection
- [ ] Model scaling/positioning controls in UI
