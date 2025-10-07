# OBJ Model Loading

## Overview
Vibe3D now supports loading 3D models from **OBJ**, **GLB**, and **glTF 2.0** files. The renderer automatically detects the file format and uses the appropriate loader.

## Supported Formats
- ✅ **OBJ** (.obj) - Wavefront OBJ format
- ✅ **GLB** (.glb) - Binary glTF 2.0
- ✅ **glTF** (.gltf) - Text glTF 2.0

## Features
- **Multi-Format Support**: Automatically detects OBJ, GLB, and glTF files
- **Vertex Deduplication**: OBJ loader optimizes mesh by removing duplicate vertices
- **Automatic Vertex Data**: Extracts positions, normals, UVs, and vertex colors
- **Material Loading**: OBJ loader supports .mtl material files (paths extracted from model directory)
- **Yaw-Only Rotation**: Model rotates smoothly around the Y-axis only
- **Floor Plane**: Automatically adds a floor plane below the model
- **Fallback to Cube**: If no model file is found, uses the default colorful cube

## How to Use

### 1. Export a Model from Blender

#### OBJ Export
1. Open Blender and create/load your model
2. Select your object(s)
3. Go to `File` → `Export` → `Wavefront (.obj)`
4. **Export Settings**:
   - Include: Check `Selection Only` (if only exporting selected)
   - Forward: `-Z Forward`
   - Up: `Y Up`
   - Geometry: Check `Apply Modifiers`, `Write Normals`, `Include UVs`
   - Objects: Check `Triangulate Faces`
5. Save as `model.obj`

#### GLB Export
1. Select your object(s)
2. Go to `File` → `Export` → `glTF 2.0 (.glb/.gltf)`
3. **Export Settings**:
   - Format: `glTF Binary (.glb)`
   - Include: Check `Selected Objects`
   - Transform: Check `+Y Up` (Vulkan coordinate system)
   - Geometry: Check `UVs`, `Normals`, `Vertex Colors`
4. Save as `model.glb`

### 2. Place the Model File
Copy your model file to the root directory of Vibe3D:
- `model.obj` (highest priority)
- `model.glb` (second priority)
- `model.gltf` (third priority)

Or copy to the build directory: `vibe3d/build/`

### 3. Run the Application
```batch
cd build
vibe3d.exe
```

The application will automatically:
- Check for `model.obj` first
- Then check for `model.glb`
- Then check for `model.gltf`
- Load and display the first model found
- Fall back to the default cube if no model is found

## Building with OBJ Support

### Requirements
- tinyobjloader library (automatically installed via vcpkg)
- tinygltf library (for GLB/glTF support)
- C++20 compiler
- CMake 3.15+

### Build Commands

#### Visual Studio Developer Command Prompt
```batch
cd build
cmake ..
ninja
```

## Model Requirements

### OBJ Format
#### Supported Features
- ✅ Multiple objects/groups
- ✅ Vertex positions (required)
- ✅ Vertex normals (optional, defaults to up vector)
- ✅ Texture coordinates (optional)
- ✅ Face indices (triangles preferred)
- ✅ Material file (.mtl) references
- ✅ Automatic vertex deduplication

#### Not Yet Supported
- ❌ Quads (will be triangulated if exported properly)
- ❌ Material textures (uses vertex colors only)
- ❌ Multiple UVs per vertex
- ❌ Per-face materials

### Coordinate System
- **Blender OBJ Export**: Z-up, Y-forward (default)
- **Vulkan/Vibe3D**: Y-up, Z-forward
- **Recommended Export**: -Z Forward, Y Up

### Vertex Structure
```cpp
struct Vertex {
    glm::vec3 position;   // X, Y, Z coordinates
    glm::vec3 normal;     // Surface normal
    glm::vec2 texCoord;   // UV coordinates (V flipped for Vulkan)
    glm::vec3 color;      // RGB vertex color (default gray)
};
```

## Example Workflow

### Blender to OBJ
1. **Create your model** in Blender
2. **Scale appropriately** (1 Blender unit ≈ 1 meter)
3. **Prepare for export**:
   - Origin to geometry: `Object` → `Set Origin` → `Origin to Geometry`
   - Apply transforms: `Object` → `Apply` → `All Transforms`
   - Triangulate (recommended): `Modifier` → `Triangulate`
4. **Export as OBJ**:
   - `File` → `Export` → `Wavefront (.obj)`
   - Forward: `-Z`, Up: `Y`
   - Triangulate Faces: ON
5. **Place `model.obj`** next to `vibe3d.exe`
6. **Run the application**

## Technical Details

### OBJ Loader Features
- **Vertex Deduplication**: Uses hash map to detect and reuse duplicate vertices
- **Memory Optimization**: Only stores unique vertex data
- **UV Coordinate Flip**: Automatically flips V coordinate (1.0 - v) for Vulkan
- **Material Directory**: Extracts directory from filepath for .mtl file loading
- **Index Types**: Supports all tinyobjloader index types
- **Console Feedback**: Reports number of shapes, vertices, and indices loaded

### Load Priority
1. `model.obj` (checked first)
2. `model.glb` (checked second)
3. `model.gltf` (checked third)
4. Default cube (fallback)

### Performance
- **OBJ Loading**: Optimized with vertex deduplication
- **GLB Loading**: Binary format loads faster than text glTF
- **Index Buffer**: Uses uint32_t for maximum compatibility

## Troubleshooting

### Model doesn't load
- Check console output for error messages
- Verify filename: `model.obj`, `model.glb`, or `model.gltf`
- Check file is in the correct directory
- Look for "OBJ Error:" messages in console

### Model appears black/dark
- OBJ files don't typically have vertex colors - model uses default gray
- Ensure proper lighting in the scene
- Check normal vectors are exported

### Model is wrong size
- Scale in Blender before export (Edit Mode: `S` → scale)
- 1 Blender unit = 1 meter in Vibe3D
- Very large models may extend beyond view frustum

### Model is rotated wrong
- Export with `-Z Forward, Y Up` settings
- Apply all transforms in Blender before export (Ctrl+A → All Transforms)
- Check object rotation in Blender (should be 0, 0, 0)

### Normals look wrong
- Ensure "Write Normals" is checked in OBJ export
- In Blender: Select mesh → Edit Mode → Mesh → Normals → Recalculate Outside
- Check for flipped normals: Viewport Overlays → Face Orientation

### UVs not working
- Ensure "Include UVs" is checked in OBJ export
- Check UV unwrap in Blender: UV Editing workspace
- Note: Current shader uses vertex colors, not textures

## Comparison: OBJ vs GLB

| Feature | OBJ | GLB/glTF |
|---------|-----|----------|
| File Size | Larger (text) | Smaller (binary) |
| Load Speed | Slower | Faster |
| Vertex Colors | Not standard | Supported |
| Materials | .mtl files | Embedded |
| Animations | Not supported | Supported |
| Binary | No | Yes (GLB) |
| Human Readable | Yes | No (GLB), Yes (glTF) |
| Industry Standard | Old | Modern |

**Recommendation**: Use **GLB** for production, **OBJ** for quick exports and testing.

## Future Enhancements
- [ ] Texture support for both OBJ and GLB
- [ ] PBR material loading from .mtl files
- [ ] Multiple model support (model switching)
- [ ] UI file picker for model selection
- [ ] Model scaling/positioning controls in UI
- [ ] Animation playback (GLB only)
- [ ] Per-material rendering

## Example Models to Test

### Simple Cube (OBJ)
```obj
# Blender v3.6.0 OBJ File
o Cube
v -1.0 -1.0 -1.0
v -1.0 -1.0 1.0
v -1.0 1.0 -1.0
v -1.0 1.0 1.0
v 1.0 -1.0 -1.0
v 1.0 -1.0 1.0
v 1.0 1.0 -1.0
v 1.0 1.0 1.0
vn -1.0 0.0 0.0
vn 0.0 -1.0 0.0
vn 1.0 0.0 0.0
vn 0.0 1.0 0.0
vn 0.0 0.0 -1.0
vn 0.0 0.0 1.0
f 2//1 3//1 1//1
f 4//2 8//2 6//2
f 6//3 7//3 5//3
f 8//4 3//4 7//4
f 1//5 5//5 7//5
f 6//6 2//6 4//6
f 2//1 4//1 3//1
f 4//2 2//2 8//2
f 6//3 8//3 7//3
f 8//4 4//4 3//4
f 1//5 3//5 5//5
f 6//6 5//6 2//6
```

### Where to Find Models
- **Blender**: Create your own
- **Sketchfab**: Download free models (check license)
- **TurboSquid**: Professional models
- **Free3D**: Free OBJ/GLB downloads
- **Poly Haven**: CC0 licensed models

---

**Note**: This renderer currently uses Forward+ rendering with PBR-style material controls in ImGui. Loaded models inherit the global material settings (albedo, roughness, metallic).
