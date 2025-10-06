# Shadow Debug View Toggle Implementation

## Status
? **Shader Updated** - `shaders/cube.frag` now supports push constant debug mode
? **Push Constants Updated** - `VulkanTypes.h` includes `debugMode` in `PushConstants`
? **InputManager Updated** - `shouldToggleShadowDebug()` function added for B key

## Remaining Implementation Steps

### 1. Add cycleShadowDebugMode() Implementation
**File**: `src/vulkan/ForwardPlusRenderer.cpp`

Add this method:
```cpp
void ForwardPlusRenderer::cycleShadowDebugMode() {
    shadowDebugMode_ = (shadowDebugMode_ + 1) % 3;
    
    const char* modes[] = {"Normal", "Shadow Factor", "Cascade Levels"};
    std::cout << "Shadow Debug Mode: " << modes[shadowDebugMode_] << std::endl;
}
```

### 2. Update Push Constants in Rendering
**File**: `src/vulkan/ForwardPlusRenderer.cpp`

In the `renderScene()` method, find where push constants are sent and update:

**Current code** (approximately line ~1500-1600):
```cpp
// Push model matrix
glm::mat4 model = ...;
vkCmdPushConstants(cmd, forwardPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 
                  0, sizeof(glm::mat4), &model);
```

**Change to**:
```cpp
// Push model matrix and debug mode
PushConstants pushConst{
    .model = model,
    .debugMode = shadowDebugMode_,
    .padding = glm::vec3(0.0f)
};
vkCmdPushConstants(cmd, forwardPipelineLayout_, 
                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                  0, sizeof(PushConstants), &pushConst);
```

### 3. Update Push Constant Range in Pipeline Creation
**File**: `src/vulkan/ForwardPlusRenderer.cpp`

In `createPipeline()` method, update the push constant range:

**Current**:
```cpp
VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(glm::mat4)
};
```

**Change to**:
```cpp
VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    .offset = 0,
    .size = sizeof(PushConstants)
};
```

### 4. Add B Key Handling in main.cpp
**File**: `main.cpp`

In the main loop, after input processing:

```cpp
// Check for shadow debug toggle (B key)
if (input->shouldToggleShadowDebug(window)) {
    vulkanRenderer->cycleShadowDebugMode();
}
```

## Debug Modes

- **Mode 0 (Normal)**: Full PBR rendering with shadows
- **Mode 1 (Shadow Factor)**: Grayscale view showing shadow intensity (black=shadow, white=lit)
- **Mode 2 (Cascade Levels)**: Color-coded cascade visualization
  - Red = Cascade 0 (nearest)
  - Green = Cascade 1  
  - Blue = Cascade 2
  - Yellow = Cascade 3 (farthest)

## Controls

Press **B** key to cycle through debug modes:
```
Normal ? Shadow Factor ? Cascade Levels ? Normal ? ...
```

## Testing

1. Run the application
2. Press B to cycle modes
3. Console will show: "Shadow Debug Mode: Normal/Shadow Factor/Cascade Levels"
4. Verify visual output matches expected mode

## Notes

- The shader is already fully implemented with all debug modes
- Push constants structure is already defined correctly
- Input handling is already in place
- Only C++ renderer code needs the implementation changes above
