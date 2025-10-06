# Vibe3D Material Controller

## Overview
The Material Controller is a real-time GUI for adjusting PBR (Physically Based Rendering) material properties in Vibe3D. It provides instant visual feedback by updating material parameters every frame through JSON-based communication.

## Features

### PBR Material Properties
- **Roughness** (0.0 - 1.0): Surface roughness for specular highlights
- **Metallic** (0.0 - 1.0): Metallic vs dielectric material behavior

### Albedo Color (RGB)
- **Red Channel** (0.0 - 1.0)
- **Green Channel** (0.0 - 1.0)
- **Blue Channel** (0.0 - 1.0)
- Real-time color preview box

### Lighting
- **Ambient Strength** (0.0 - 0.1): Intensity of ambient lighting
- **Light Intensity** (0.0 - 20.0): Directional light brightness

### Emissive (Self-Illumination)
- **Emissive Red** (0.0 - 1.0)
- **Emissive Green** (0.0 - 1.0)
- **Emissive Blue** (0.0 - 1.0)
- **Emissive Strength** (0.0 - 10.0): Self-illumination multiplier
- Real-time emissive preview

### Material Presets
Quick-select preset materials:
- **Rough Plastic**: Matte plastic appearance
- **Smooth Plastic**: Shiny plastic surface
- **Rough Metal**: Brushed metal effect
- **Polished Metal**: Mirror-like metal
- **Gold**: Realistic gold material
- **Copper**: Authentic copper appearance
- **Chrome**: Highly reflective chrome
- **Rubber**: Matte rubber surface
- **Wood**: Natural wood finish

## Usage

### Quick Start
Run the batch file to launch both the GUI and application:
```cmd
.\run_with_material_controller.bat
```

### Manual Launch
1. Start the Material Controller:
```cmd
py material_controller.py
```

2. Start Vibe3D:
```cmd
cd build
vibe3d.exe
```

### Controls
- **Sliders**: Drag to adjust values in real-time
- **Spinboxes**: Type exact values with precision control
- **Preset Dropdown**: Select pre-configured materials
- **Reset to Defaults**: Restore original default values
- **Save Config**: Manually save current settings to JSON

## Technical Details

### Architecture
- **GUI Framework**: PyQt6
- **Communication**: JSON file (`material_config.json`)
- **Update Frequency**: Auto-save every 100ms
- **Integration**: C++ reads JSON every frame, zero performance impact
- **Shader Integration**: Material properties passed via `MaterialUBO` uniform buffer

### Default Values
```json
{
  "roughness": 0.5,
  "metallic": 0.0,
  "albedoR": 0.8,
  "albedoG": 0.3,
  "albedoB": 0.2,
  "ambientStrength": 0.001,
  "lightIntensity": 8.0,
  "emissiveR": 0.0,
  "emissiveG": 0.0,
  "emissiveB": 0.0,
  "emissiveStrength": 0.0
}
```

### Files
- `material_controller.py`: PyQt6 GUI application
- `material_config.json`: Configuration file (auto-saved)
- `run_with_material_controller.bat`: Convenience launcher
- `src/vulkan/ForwardPlusRenderer.cpp`: C++ integration
- `src/vulkan/VulkanTypes.h`: `MaterialUBO` structure definition
- `shaders/cube.frag`: Shader that uses material properties

## Tips & Tricks

### Experimenting with Materials
1. **Metallic Materials**: Set metallic to 1.0, adjust roughness for different metal types
2. **Dielectric Materials**: Keep metallic at 0.0, use roughness for shine level
3. **Glowing Objects**: Increase emissive strength with desired emissive color
4. **Dark Scenes**: Lower ambient strength for more dramatic shadows
5. **Bright Scenes**: Increase light intensity for outdoor/daylight effects

### Performance
- No performance impact - JSON reading is negligible
- Auto-save occurs every 100ms when values change
- Changes apply immediately to running application

### Workflow
1. Start with a preset close to desired look
2. Fine-tune individual parameters
3. Observe changes in real-time on 3D model
4. Save final configuration manually if desired

## Dependencies
- Python 3.x
- PyQt6 (`pip install PyQt6`)
- nlohmann-json (C++ library, installed via vcpkg)

## Notes
- Material properties are shared across all rendered objects
- Changes persist in `material_config.json` between sessions
- Emissive materials bypass shadow calculations (self-illuminated)
- Preset selection automatically saves configuration

## See Also
- `SHADOW_BIAS_CONTROLLER_README.md` - Shadow tuning GUI
- `VULKAN_ARCHITECTURE.md` - Renderer architecture
- Material config is independent of shadow config
