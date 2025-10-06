# Shadow System Diagnostic & Improvements

## Current Status ?

The shadow system is **fully implemented** and working with:
- ? 4 cascade levels (0.98m, 3.28m, 11.85m, 50m)
- ? 2048x2048 resolution per cascade
- ? PCF filtering (3x3 kernel)
- ? Depth bias enabled
- ? Debug modes functional (B key)
- ? Running at ~6000 FPS

## Current Configuration

### Light Direction
```cpp
lightDirection_ = glm::vec3(0.0f, -1.0f, 0.0f); // Pointing straight down
```

**This means:**
- Light is directly above (like noon sun)
- Shadows will be cast straight down
- Minimal shadow visibility on the cube's top face

### Scene Setup
```cpp
// Cube position
cubeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)); // 0.5m above floor

// Floor position
floorModel = glm::mat4(1.0f); // At y = -2.0 (from geometry)
```

## Why Shadows May Be Hard to See

### 1. Light Direction Issue
With light pointing straight down `(0, -1, 0)`:
- **Pro**: Realistic noon sun
- **Con**: Shadows are cast straight down under objects
- **Con**: Top surfaces are fully lit (no self-shadowing visible)

### 2. Camera Angle
```cpp
cameraPos = glm::vec3(0, 0, 4.0f); // Looking from front
```
- Camera is at same height as cube (y=0, cube at y=0.5)
- Hard to see shadows directly below objects

## Recommended Improvements

### Improvement #1: Adjust Light Direction (Angled Light)

**Change light to come from an angle:**

```cpp
// In ForwardPlusRenderer.h, change:
glm::vec3 lightDirection_ = glm::vec3(-0.5f, -1.0f, -0.3f); // Angled from upper-left
```

**Benefits:**
- Shadows will be cast at an angle
- More visible from camera position
- Better depth perception
- Classic 3D demo lighting

### Improvement #2: Add Light Direction Control

**Add to ForwardPlusRenderer.h:**
```cpp
void setLightDirection(const glm::vec3& direction) {
    lightDirection_ = glm::normalize(direction);
}
```

**Add to main.cpp:**
```cpp
// After initialization:
vulkanRenderer->setLightDirection(glm::vec3(-0.5f, -1.0f, -0.3f));
```

### Improvement #3: Increase Shadow Contrast

**In cube.frag, change ambient lighting:**

```glsl
// Current:
vec3 ambient = vec3(0.005) * albedo; // Very dark

// Change to:
vec3 ambient = vec3(0.02) * albedo; // Slightly brighter, better shadow visibility
```

**Or reduce light intensity:**
```glsl
// Current:
vec3 radiance = vec3(4.0); // Very bright

// Change to:
vec3 radiance = vec3(2.5); // Moderate, better shadow contrast
```

### Improvement #4: Adjust Camera for Better Shadow View

**In main.cpp:**
```cpp
// Current:
glm::vec3 cameraPos(0, 0, 4.0f);

// Better view:
glm::vec3 cameraPos(2.0f, 1.5f, 4.0f); // Higher and to the side
```

### Improvement #5: Enable Shadow Debug Mode

**Press B key twice to see cascade visualization:**
- Mode 0: Normal (current)
- Mode 1: Shadow Factor (grayscale - shows shadow coverage)
- Mode 2: Cascade Levels (color-coded cascades)

## Quick Test: Are Shadows Working?

### Test 1: Shadow Factor Mode
1. Press **B** once
2. You should see:
   - **Black areas** = Shadowed
   - **White areas** = Lit
   - **Gray areas** = Soft shadows (PCF)

### Test 2: Cascade Visualization
1. Press **B** twice
2. You should see colored regions:
   - **Red** = Nearest cascade (highest detail)
   - **Green** = Mid-range cascade
   - **Blue** = Far cascade
   - **Yellow** = Farthest cascade

### Test 3: Check Console Output
Look for shadow debug messages when pressing B:
```
[DEBUG] B key pressed - toggling shadow debug
[MAIN] Calling cycleShadowDebugMode()
==> Shadow Debug Mode: Shadow Factor (mode=1)
```

## Implementation: Angled Light Direction

**File: `src/vulkan/ForwardPlusRenderer.h`**

Change line with `lightDirection_`:
```cpp
// OLD:
glm::vec3 lightDirection_ = glm::vec3(0.0f, -1.0f, 0.0f);

// NEW (angled from upper front-left):
glm::vec3 lightDirection_ = glm::vec3(-0.5f, -1.0f, -0.3f);
```

## Shadow Visibility Checklist

- [ ] Light direction is angled (not straight down)
- [ ] Camera can see shadow-receiving surfaces
- [ ] Ambient lighting is low enough
- [ ] Shadow maps are being rendered (`renderShadowCascades` called)
- [ ] Shadow bias is appropriate (not too high)
- [ ] PCF is working (soft shadow edges)
- [ ] Debug mode shows shadows (B key, mode 1)

## Advanced: Dynamic Light Direction

**Add this to ForwardPlusRenderer.h:**
```cpp
public:
    void setLightDirection(const glm::vec3& direction) {
        lightDirection_ = glm::normalize(direction);
    }
    
    const glm::vec3& getLightDirection() const {
        return lightDirection_;
    }
```

**Add keyboard control in main.cpp:**
```cpp
// In main loop:
if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    auto dir = vulkanRenderer->getLightDirection();
    vulkanRenderer->setLightDirection(dir + glm::vec3(-0.01f, 0, 0));
}
if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    auto dir = vulkanRenderer->getLightDirection();
    vulkanRenderer->setLightDirection(dir + glm::vec3(0.01f, 0, 0));
}
```

## Expected Visual Result

With angled light `(-0.5, -1.0, -0.3)`:
- Cube casts shadow **diagonally** across the floor
- Shadow is **visible from front camera**
- Shadow has **soft edges** (PCF working)
- **Self-shadowing** visible on cube faces
- Floor shows clear **dark shadow region**

## Performance Notes

Current performance (~6000 FPS) indicates:
- ? Shadow maps are being generated efficiently
- ? PCF filtering is not a bottleneck
- ? 4 cascades running smoothly
- ? 2048x2048 resolution is optimal

## Troubleshooting

### "I don't see any shadows"
1. Press **B** once to see shadow factor mode
2. If still no shadows visible:
   - Check light direction is not (0, -1, 0)
   - Verify shadow UBO is being updated
   - Check shadow maps are bound correctly

### "Shadows are too dark"
- Increase ambient: `vec3(0.02)` ? `vec3(0.05)`
- Reduce light intensity: `vec3(4.0)` ? `vec3(2.0)`

### "Shadows are too light/washed out"
- Decrease ambient: `vec3(0.005)` ? `vec3(0.001)`
- Increase bias slightly (may add peter-panning)

### "Shadow edges are jagged"
- Increase PCF kernel size from 3x3 to 5x5
- Increase shadow map resolution from 2048 to 4096

## Next Steps

1. **Change light direction** to angled (highest impact)
2. **Test with B key** to verify shadows are working
3. **Adjust camera position** for better view
4. **Fine-tune ambient/intensity** for contrast
5. **Add light direction control** for experimentation

## Code Changes Summary

**Minimal change for immediate results:**

```cpp
// src/vulkan/ForwardPlusRenderer.h (line ~140)
glm::vec3 lightDirection_ = glm::vec3(-0.5f, -1.0f, -0.3f); // Change this line
```

**Rebuild:**
```bash
cmake --build build --config Release
```

**Test:**
```bash
.\build\Release\vibe3d.exe
# Press B to cycle debug modes
```

---

## Current Shadow System Architecture

```
Shadow Rendering Flow:
1. updateShadowUBO() - Calculate light space matrices
2. renderShadowCascades() - Render depth from light POV
   ?? Cascade 0: 0.1m ? 0.98m (Red in debug)
   ?? Cascade 1: 0.98m ? 3.28m (Green in debug)
   ?? Cascade 2: 3.28m ? 11.85m (Blue in debug)
   ?? Cascade 3: 11.85m ? 50m (Yellow in debug)
3. Main rendering - Sample shadow maps with PCF
   ?? selectCascadeIndex() - Choose cascade based on depth
   ?? calculateShadowPCF() - 3x3 PCF filtering
   ?? Apply shadow factor to lighting
```

The system is **fully functional** - shadows just need better configuration for visibility!
