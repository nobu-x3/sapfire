# Cross-Platform Shader Guide (D3D12 + Vulkan)

This guide explains how to write HLSL shaders that work on both D3D12 and Vulkan, especially for bindless rendering.

## The Problem

D3D12 Shader Model 6.6+ uses `ResourceDescriptorHeap` for bindless resources:

```hlsl
// D3D12 SM6.6 only
StructuredBuffer<float3> buffer = ResourceDescriptorHeap[index];
```

**This doesn't work in Vulkan!** Vulkan requires explicit descriptor bindings.

## Solution 1: Vulkan HLSL with Descriptor Arrays (Recommended)

Use Vulkan-specific HLSL attributes with **unbounded descriptor arrays**:

```hlsl
// Vulkan HLSL - works with DXC -spirv
[[vk::binding(0, 2)]] StructuredBuffer<float3> positionBuffers[] : register(t0, space2);

// Access using index
StructuredBuffer<float3> pos = positionBuffers[index];
```

### Complete Example

See `assets/shaders/bindless_vulkan.hlsl` for a full working example with:
- Push constants for per-object indices
- Descriptor sets for different update frequencies
- Unbounded resource arrays for bindless textures/buffers
- `NonUniformResourceIndex()` for dynamic indexing

### Advantages
✅ True bindless rendering in Vulkan
✅ Clean, readable syntax
✅ Type-safe
✅ Works with DXC SPIR-V compiler

### Disadvantages
❌ Vulkan-specific (won't compile for D3D12)
❌ Requires separate shader files for D3D12

## Solution 2: Preprocessor Macros (Cross-Platform Single File)

Use preprocessor to switch between D3D12 and Vulkan:

```hlsl
#ifdef VULKAN
    // Vulkan bindless with descriptor arrays
    [[vk::binding(0, 2)]] StructuredBuffer<float3> positionBuffers[];
    #define GET_POSITION_BUFFER(idx) positionBuffers[idx]
#else
    // D3D12 bindless with ResourceDescriptorHeap
    #define GET_POSITION_BUFFER(idx) ResourceDescriptorHeap[idx]
#endif

// Use in code
VS_OUTPUT VS(uint vertexID : SV_VertexID) {
    StructuredBuffer<float3> positions = GET_POSITION_BUFFER(bufferIndex);
    // ...
}
```

Compile:
```bash
# For Vulkan
dxc -spirv -D VULKAN -T vs_6_0 -E VS shader.hlsl -Fo shader.vert.spv

# For D3D12
dxc -T vs_6_6 -E VS shader.hlsl -Fo shader.cso
```

### Advantages
✅ Single shader file for both APIs
✅ Maintainable - changes affect both

### Disadvantages
❌ More complex preprocessor logic
❌ Need to compile twice with different defines

## Solution 3: Separate Files (Cleanest)

Maintain separate shaders:
- `bindless.hlsl` - D3D12 version with ResourceDescriptorHeap
- `bindless_vulkan.hlsl` - Vulkan version with descriptor arrays

### Advantages
✅ API-specific optimizations
✅ Clearest code (no preprocessor)
✅ Can use best features of each API

### Disadvantages
❌ Code duplication
❌ Must sync changes manually

## Vulkan Bindless Architecture

### Descriptor Set Layout

Organize descriptors by update frequency:

```
Set 0: Per-frame (camera, scene data)
  ├─ binding 0: Scene uniform buffer
  └─ binding 1: Pass uniform buffer

Set 1: Per-object (push constants)
  └─ Push constant: object indices + matrices

Set 2: Bindless resources (rarely updated)
  ├─ binding 0: StructuredBuffer<float3> positions[]
  ├─ binding 1: StructuredBuffer<float3> normals[]
  ├─ binding 2: StructuredBuffer<float2> uvs[]
  ├─ binding 3: Texture2D textures[]
  └─ binding 4: SamplerState samplers[]

Set 3: Material data (per-material)
  └─ binding 0: StructuredBuffer<MaterialData> materials
```

### Push Constants

Push constants are perfect for per-draw indices:

```hlsl
[[vk::push_constant]]
struct PushConstants {
    uint posBufferIndex;
    uint normalBufferIndex;
    uint materialIndex;
    float4x4 modelMatrix;
} pushConstants;
```

C++ side:
```cpp
vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &data);
```

### Unbounded Arrays

Vulkan 1.2+ supports unbounded descriptor arrays:

```hlsl
// Unbounded array (no size limit)
[[vk::binding(0, 2)]] Texture2D textures[] : register(t0, space2);
```

Requires Vulkan features:
- `VkPhysicalDeviceDescriptorIndexingFeatures::descriptorBindingVariableDescriptorCount`
- `VkPhysicalDeviceDescriptorIndexingFeatures::runtimeDescriptorArray`

### NonUniformResourceIndex

Use for dynamic texture indexing:

```hlsl
// Material has dynamic texture index
uint texIndex = material.albedoTexIndex;

// Use NonUniformResourceIndex for safety
Texture2D tex = textures[NonUniformResourceIndex(texIndex)];
float4 color = tex.Sample(sampler, uv);
```

This tells the compiler the index isn't uniform across the draw call.

## Compilation

### Vulkan (SPIR-V)

```bash
dxc -spirv -T vs_6_0 -E VS bindless_vulkan.hlsl -Fo bindless_vulkan.vert.spv
dxc -spirv -T ps_6_0 -E PS bindless_vulkan.hlsl -Fo bindless_vulkan.frag.spv
```

### D3D12

```bash
dxc -T vs_6_6 -E VS bindless.hlsl -Fo bindless_vs.cso
dxc -T ps_6_6 -E PS bindless.hlsl -Fo bindless_ps.cso
```

## Required Extensions/Features

### Vulkan Extensions
Enable these for bindless:
- `VK_EXT_descriptor_indexing` (core in Vulkan 1.2)
- `VK_KHR_buffer_device_address` (for advanced use cases)

### Device Features
```cpp
VkPhysicalDeviceDescriptorIndexingFeatures indexing{};
indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
indexing.runtimeDescriptorArray = VK_TRUE;
indexing.descriptorBindingPartiallyBound = VK_TRUE;
indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
indexing.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
```

## Recommended Approach

**For new projects:**
Use **Solution 3 (Separate Files)** with:
- `*_vulkan.hlsl` for Vulkan bindless shaders
- `*.hlsl` for D3D12 shaders

**For existing D3D12 projects:**
Use **Solution 2 (Preprocessor)** to add Vulkan support gradually.

**For simple shaders:**
Write them API-agnostic from the start:
- Use explicit register bindings
- Avoid ResourceDescriptorHeap
- Use basic textures/buffers

## Examples

Check these example shaders:
- `bindless_vulkan.hlsl` - Full Vulkan bindless example
- `color.hlsl` - Simple API-agnostic shader
- `default.hlsl` - Basic PBR without bindless

## Further Reading

- [Vulkan HLSL - GitHub](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst)
- [DXC SPIR-V Documentation](https://github.com/microsoft/DirectXShaderCompiler/wiki/SPIR%E2%80%90V-CodeGen)
- [Vulkan Descriptor Indexing](https://www.khronos.org/blog/vulkan-descriptor-indexing)
