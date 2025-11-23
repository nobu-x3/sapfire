# Shader Compilation Guide

## Overview

Sapfire Engine uses SPIR-V shaders for the Vulkan backend. HLSL shaders in `assets/shaders/` must be compiled to SPIR-V (`.spv` files) before use.

## Requirements

You need one of the following shader compilers:

### DXC (Recommended for HLSL)
- **Linux**: `sudo pacman -S directx-shader-compiler` (Arch) or build from source
- **Windows**: Included in `engine/vendor/dxc/`
- **macOS**: `brew install dxc`

### glslc (Alternative, from Vulkan SDK)
- Download Vulkan SDK: https://vulkan.lunarg.com/sdk/home
- Includes `glslc` compiler
- Note: Limited HLSL support

## Compiling Shaders

### Option 1: Manual Compilation (Recommended)

Run the compilation script manually:

```bash
# From project root
./compile_shaders.sh

# Or specify output directory
./compile_shaders.sh /path/to/output
```

The script will:
- Find all `.hlsl` files in `assets/shaders/`
- Detect shader types (vertex, fragment, compute) by entry points
- Compile to `.spv` files in `build/assets/shaders/`

### Option 2: CMake Target

Compile shaders using CMake:

```bash
cmake --build build --target compile_shaders
```

### Option 3: Auto-compile During Build

Edit `CMakeLists.txt` and uncomment:

```cmake
add_dependencies(Sandbox compile_shaders)
```

This will automatically compile shaders every time you build the project.

## Shader Entry Points

The compilation script looks for these entry points in HLSL shaders:

| Shader Type | Entry Point | Output Extension |
|-------------|-------------|------------------|
| Vertex      | `VS()`      | `.vert.spv`      |
| Fragment    | `PS()`      | `.frag.spv`      |
| Compute     | `CS()`      | `.comp.spv`      |

### Example HLSL Shader

```hlsl
// assets/shaders/triangle.hlsl

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Vertex Shader Entry Point
VS_OUTPUT VS(uint vertexID : SV_VertexID) {
    VS_OUTPUT output;
    // ... shader code
    return output;
}

// Fragment Shader Entry Point
float4 PS(VS_OUTPUT input) : SV_TARGET {
    return input.color;
}
```

This will compile to:
- `build/assets/shaders/triangle.vert.spv`
- `build/assets/shaders/triangle.frag.spv`

## Using Compiled Shaders

In your C++ code:

```cpp
GraphicsPipelineStateDesc pipeline_desc{};
pipeline_desc.shader_module.vertex_shader_path = L"triangle.vert";  // .spv auto-appended
pipeline_desc.shader_module.pixel_shader_path = L"triangle.frag";   // .spv auto-appended
pipeline_desc.shader_module.vertex_entry_point = L"main";
pipeline_desc.shader_module.pixel_entry_point = L"main";

auto* pipeline = device->create_graphics_pipeline(pipeline_desc);
```

The Vulkan shader loader automatically:
- Searches in the asset directory
- Appends `.spv` extension if not present
- Loads the SPIR-V binary

## Cross-Platform Shaders (D3D12 + Vulkan)

Some shaders use D3D12-specific features that aren't compatible with Vulkan:

- `ResourceDescriptorHeap` (D3D12 SM6.6 dynamic indexing)
- D3D12-specific root signatures

The compilation script automatically **skips** these shaders with a warning.

### Writing Cross-Platform Bindless Shaders

For detailed information on writing HLSL shaders that work on both D3D12 and Vulkan, including bindless rendering techniques, see:

**[CROSS_PLATFORM_SHADERS.md](CROSS_PLATFORM_SHADERS.md)**

This guide covers:
- Vulkan bindless rendering with descriptor arrays
- Push constants for per-object data
- Three approaches: Vulkan-only, preprocessor macros, or separate files
- Complete descriptor set layout recommendations
- Required Vulkan extensions and features
- Working example: `assets/shaders/bindless_vulkan.hlsl`

## Troubleshooting

### "Skipping D3D12-specific shader"
This is normal - the shader uses D3D12-only features. Create a Vulkan version or rewrite using Vulkan-compatible HLSL.

### "No shader compiler found"
Install DXC or Vulkan SDK with glslc.

### "Shader failed to compile"
Check shader syntax:
```bash
# Manual test with dxc
dxc -spirv -T vs_6_0 -E VS assets/shaders/myshader.hlsl -Fo /tmp/test.spv
```

### "Failed to load SPIR-V shader"
- Ensure shaders are compiled before running
- Check shader paths match compiled `.spv` files
- Run `./compile_shaders.sh` to regenerate

## Asset Directory Structure

```
sapfire/
├── assets/
│   └── shaders/
│       ├── triangle.hlsl       # Source HLSL
│       ├── pbr.hlsl
│       └── ...
└── build/
    └── assets/
        └── shaders/
            ├── triangle.vert.spv  # Compiled SPIR-V
            ├── triangle.frag.spv
            ├── pbr.vert.spv
            ├── pbr.frag.spv
            └── ...
```

Assets are automatically copied to `build/assets/` during build, and shaders are compiled to `build/assets/shaders/`.
