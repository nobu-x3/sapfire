# Sapfire Render API Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Concepts](#core-concepts)
4. [Getting Started Tutorial](#getting-started-tutorial)
5. [Intermediate Tutorial: Rendering with Textures](#intermediate-tutorial-rendering-with-textures)
6. [Advanced Tutorial: Bindless Rendering](#advanced-tutorial-bindless-rendering)
7. [API Reference](#api-reference)
8. [Best Practices](#best-practices)

---

## Overview

The Sapfire Render API is a **stateless, backend-agnostic** graphics abstraction layer designed for modern rendering techniques. It provides a unified interface across DirectX 12 and Vulkan while exposing low-level control for high-performance applications.

### Key Features
- **Stateless Design**: Device is purely a factory - no hidden state
- **Explicit Resource Ownership**: User owns all created resources via `stl::unique_ptr`
- **Explicit Synchronization**: User-controlled fences and semaphores
- **Bindless Resources**: Modern descriptor-free resource access
- **Unified API**: Same code works on DX12 and Vulkan
- **Performance-First**: Zero-overhead abstraction principles

### Design Philosophy

The API follows these principles:
1. **No Magic**: All operations are explicit and predictable
2. **User Control**: Applications manage all resources and synchronization
3. **Modern GPU Features**: Designed for bindless, GPU-driven rendering
4. **Backend Agnostic**: Abstract concepts that map cleanly to both DX12 and Vulkan

---

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  (GameContext, AssetManager, BindlessResourceRegistry)      │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│                  Render Backend API                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │IGraphicsDevice│  │ ICommandQueue│  │   IContext   │      │
│  │  (Factory)   │  │              │  │              │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │IDescriptorPool│  │IDescriptorSet│  │ IFence/ISem  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└────────────────────┬────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
┌───────▼────────┐      ┌─────────▼────────┐
│ Vulkan Backend │      │  DX12 Backend    │
└────────────────┘      └──────────────────┘
```

### Resource Ownership Model

The **stateless design** means:
- **IGraphicsDevice**: Factory for all resources, owns only native device handle and swapchain
- **Application**: Owns all created resources (queues, contexts, pipelines, buffers, textures)
- **No Hidden State**: Device has no internal context, queue, or pipeline state
- **Explicit Lifetime Management**: Resources are returned as `stl::unique_ptr` and destroyed when released

This is fundamentally different from traditional graphics APIs where devices maintain internal state.

---

## Core Concepts

### 1. Stateless Device Factory

The device is purely a resource factory:

```cpp
// Device creates resources but doesn't own them
auto queue = device->create_direct_queue("Main Queue");
auto context = device->create_graphics_context();
auto allocator = device->create_memory_allocator();

// User owns and manages lifetime
stl::unique_ptr<ICommandQueue> m_Queue = std::move(*queue);
```

### 2. Result Types

All factory methods return `stl::result<T>` for error handling:

```cpp
auto pipeline_result = device->create_graphics_pipeline(desc);
if (!pipeline_result) {
    // Handle error
    CORE_ERROR("Failed to create pipeline: {}", pipeline_result.error().c_str());
    return;
}
m_Pipeline = std::move(*pipeline_result);
```

### 3. Render Passes and Framebuffers

Modern explicit render pass model:

```cpp
// Define render pass
auto render_pass = device->create_render_pass({
    .color_attachments = {mem::MemTag::Temp, 1, {
        .format = Format::RGBA16_FLOAT,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
        .initial_layout = ResourceState::Undefined,
        .final_layout = ResourceState::RenderTarget,
    }},
    .depth_attachment = depth_desc,
    .name = "Main Render Pass",
});

// Create framebuffer for each swapchain image
for (u32 i = 0; i < device->get_back_buffer_count(); ++i) {
    auto framebuffer = device->create_framebuffer({
        .render_pass = render_pass.get(),
        .color_attachments = {&device->get_back_buffer(i)},
        .depth_attachment = &depth_texture,
        .width = 1920,
        .height = 1080,
        .name = "Main Framebuffer",
    });
}
```

### 4. Pipeline Layouts

Pipeline layouts define descriptor sets and push constants:

```cpp
PipelineLayoutDesc layout_desc{};

// Define descriptor sets
DescriptorSetLayout set0{};
set0.bindings.push_back({
    .binding = 0,
    .type = DescriptorType::UniformBuffer,
    .count = 1,
    .stages = ShaderStage::Vertex,
});

layout_desc.descriptor_set_layouts.push_back(std::move(set0));

// Define push constants
layout_desc.push_constant_ranges.push_back({
    .stages = ShaderStage::AllGraphics,
    .offset = 0,
    .size = sizeof(PushConstants),
});

auto pipeline_layout = device->create_pipeline_layout(layout_desc);
```

### 5. Memory Allocation

Resources are allocated through `IMemoryAllocator`:

```cpp
// Create allocator
auto allocator = device->create_memory_allocator();

// Allocate buffer
auto buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Vertex,
    .size_in_bytes = sizeof(vertices),
    .name = "Vertex Buffer",
});

// Update data
buffer->update(vertices, sizeof(vertices));

// Allocate texture
auto texture = allocator->allocate_texture({
    .usage = TextureUsage::ShaderResource,
    .format = Format::RGBA8_UNORM,
    .width = 256,
    .height = 256,
    .name = "Albedo Texture",
});
```

### 6. Descriptor Sets

Descriptor sets bind resources to shaders:

```cpp
// Create descriptor pool
auto pool = device->create_descriptor_pool({
    .max_sets = 100,
    .max_uniform_buffers = 1000,
    .max_storage_buffers = 10000,
    .max_sampled_images = 10000,
    .name = "Main Descriptor Pool",
});

// Define descriptor set layout
DescriptorSetLayout layout{};
layout.bindings.push_back({
    .binding = 0,
    .type = DescriptorType::UniformBuffer,
    .count = 1,
    .stages = ShaderStage::Vertex,
});

// Allocate descriptor set
auto descriptor_set = pool->allocate_set(layout);

// Write descriptors
descriptor_set->write_buffer(0, *uniform_buffer);
descriptor_set->write_texture(1, *texture, sampler.get());
```

### 7. Synchronization

Explicit synchronization using fences and semaphores:

```cpp
// Create sync objects
auto fence = device->create_fence(true, "Frame Fence");
auto image_available = device->create_semaphore("Image Available");
auto render_finished = device->create_semaphore("Render Finished");

// Wait for previous frame
fence->wait();

// Acquire swapchain image
u32 image_index = device->acquire_next_image(image_available.get());

// Submit work
queue->submit({
    .wait_semaphores = {image_available.get()},
    .command_contexts = {context.get()},
    .signal_semaphores = {render_finished.get()},
    .signal_fence = fence.get(),
});

// Present
device->present({render_finished.get()});
```

### 8. Memory Tags

**CRITICAL**: All Sapfire STL containers **must** be initialized with a `mem::MemTag`:

```cpp
// ✅ CORRECT - Persistent data
stl::vector<Buffer> m_Buffers{mem::MemTag::Render};
stl::unordered_map<stl::string, Texture> m_Textures{mem::MemTag::Render};

// ✅ CORRECT - Temporary/local scope
stl::string temp_name(mem::MemTag::Temp, "Temporary Name");
stl::vector<u32> temp_indices(mem::MemTag::Temp);

// ❌ INCORRECT - Will cause assertion failure!
stl::vector<Buffer> m_Buffers;  // Missing memory tag!
stl::string name = "Test";      // Missing memory tag!
```

**Memory Tag Guidelines:**
- `mem::MemTag::Render` - Persistent rendering resources
- `mem::MemTag::Temp` - Temporary containers with current scope lifetime
- `mem::MemTag::Application` - Application-level data
- `mem::MemTag::Logic` - Game logic data
- `mem::MemTag::Strings` - String data that needs to persist

---

## Getting Started Tutorial

This tutorial shows how to set up a basic rendering pipeline and draw a colored triangle.

### Prerequisites

You'll need:
- SDL3 for window creation
- Vulkan SDK or DirectX 12 SDK
- Sapfire engine

### Step 1: Initialize the Render Backend

```cpp
#include <render/render_backend.h>
#include <render/i_graphics_device.h>
#include <SDL3/SDL.h>

using namespace sf::render;

// Initialize SDL
SDL_Init(SDL_INIT_VIDEO);
SDL_Window* window = SDL_CreateWindow(
    "Sapfire Tutorial",
    1280, 720,
    SDL_WINDOW_VULKAN
);

// Initialize render backend
#ifdef SF_PLATFORM_WINDOWS
    RenderBackend::initialize(RenderAPI::DX12);
#else
    RenderBackend::initialize(RenderAPI::Vulkan);
#endif

// Create graphics device
auto device_result = RenderBackend::create_device({
    .window_handle = window,
    .width = 1280,
    .height = 720,
    .buffer_count = 3,
    .format = Format::RGBA16_FLOAT,
    .refresh_rate = 60,
});

if (!device_result) {
    std::cerr << "Failed to create device: " << device_result.error() << std::endl;
    return -1;
}

stl::unique_ptr<IGraphicsDevice> device = std::move(*device_result);
```

### Step 2: Create Essential Resources

```cpp
// Create command queue
auto queue_result = device->create_direct_queue("Main Queue");
if (!queue_result) {
    std::cerr << "Failed to create queue: " << queue_result.error() << std::endl;
    return -1;
}
stl::unique_ptr<ICommandQueue> queue = std::move(*queue_result);

// Create graphics context
auto context_result = device->create_graphics_context();
if (!context_result) {
    std::cerr << "Failed to create context: " << context_result.error() << std::endl;
    return -1;
}
stl::unique_ptr<IGraphicsContext> context = std::move(*context_result);

// Create memory allocator
auto allocator_result = device->create_memory_allocator();
if (!allocator_result) {
    std::cerr << "Failed to create allocator: " << allocator_result.error() << std::endl;
    return -1;
}
stl::unique_ptr<IMemoryAllocator> allocator = std::move(*allocator_result);

// Create synchronization objects for double buffering
constexpr u32 MAX_FRAMES = 2;
stl::vector<stl::unique_ptr<IFence>> fences{mem::MemTag::Render};
stl::vector<stl::unique_ptr<ISemaphore>> image_available_sems{mem::MemTag::Render};
stl::vector<stl::unique_ptr<ISemaphore>> render_finished_sems{mem::MemTag::Render};

for (u32 i = 0; i < MAX_FRAMES; ++i) {
    stl::string fence_name(mem::MemTag::Temp, "Frame Fence ");
    fence_name.append(std::to_string(i));

    auto fence_result = device->create_fence(true, fence_name.c_str());
    if (!fence_result) {
        std::cerr << "Failed to create fence: " << fence_result.error() << std::endl;
        return -1;
    }
    fences.push_back(std::move(*fence_result));

    auto img_sem_result = device->create_semaphore("Image Available");
    image_available_sems.push_back(std::move(*img_sem_result));

    auto render_sem_result = device->create_semaphore("Render Finished");
    render_finished_sems.push_back(std::move(*render_sem_result));
}
```

### Step 3: Create Render Pass and Framebuffers

```cpp
// Create render pass
RenderPassDesc::AttachmentDesc color_attachment{
    .format = device->get_back_buffer(0).format,
    .load_op = LoadOp::Clear,
    .store_op = StoreOp::Store,
    .initial_layout = ResourceState::Undefined,
    .final_layout = ResourceState::RenderTarget,
};

auto render_pass_result = device->create_render_pass({
    .color_attachments = {mem::MemTag::Temp, 1, color_attachment},
    .name = "Main Render Pass",
});

if (!render_pass_result) {
    std::cerr << "Failed to create render pass: " << render_pass_result.error() << std::endl;
    return -1;
}
stl::unique_ptr<IRenderPass> render_pass = std::move(*render_pass_result);

// Create framebuffers for each swapchain image
stl::vector<stl::unique_ptr<IFramebuffer>> framebuffers{mem::MemTag::Render};
for (u32 i = 0; i < device->get_back_buffer_count(); ++i) {
    stl::array<Texture*, 1> attachments{&device->get_back_buffer(i)};

    auto fb_result = device->create_framebuffer({
        .render_pass = render_pass.get(),
        .color_attachments = attachments,
        .depth_attachment = nullptr,
        .width = 1280,
        .height = 720,
        .name = "Main Framebuffer",
    });

    if (!fb_result) {
        std::cerr << "Failed to create framebuffer: " << fb_result.error() << std::endl;
        return -1;
    }
    framebuffers.push_back(std::move(*fb_result));
}
```

### Step 4: Create Pipeline Layout (No Descriptors)

```cpp
// Simple pipeline layout with no descriptor sets
PipelineLayoutDesc layout_desc{
    .name = "Basic Pipeline Layout",
};

auto layout_result = device->create_pipeline_layout(layout_desc);
if (!layout_result) {
    std::cerr << "Failed to create pipeline layout: " << layout_result.error() << std::endl;
    return -1;
}
stl::unique_ptr<IPipelineLayout> pipeline_layout = std::move(*layout_result);
```

### Step 5: Create Shaders and Pipeline

```cpp
// Load shaders (pre-compiled SPIR-V)
auto vs_result = device->create_shader({
    .stage = ShaderStage::Vertex,
    .file_path = "shaders/basic.vert.spv",
    .entry_point = "VS",
});

auto ps_result = device->create_shader({
    .stage = ShaderStage::Pixel,
    .file_path = "shaders/basic.frag.spv",
    .entry_point = "PS",
});

// Create graphics pipeline
GraphicsPipelineDesc pipeline_desc{
    .vertex_shader = vs_result->get(),
    .pixel_shader = ps_result->get(),
    .layout = pipeline_layout.get(),
    .render_pass = render_pass.get(),
    .rasterizer = {
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::CounterClockwise,
    },
    .depth_stencil = {
        .depth_test = false,
        .depth_write = false,
    },
    .name = "Basic Pipeline",
};

auto pipeline_result = device->create_graphics_pipeline(pipeline_desc);
stl::unique_ptr<IPipeline> pipeline = std::move(*pipeline_result);
```

### Step 6: Main Render Loop

```cpp
u32 frame_index = 0;

while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) running = false;
    }

    // Wait for previous frame
    fences[frame_index]->wait();
    fences[frame_index]->reset();

    // Acquire swapchain image
    auto image_result = device->acquire_next_image(image_available_sems[frame_index].get());
    if (!image_result) continue;
    u32 image_index = *image_result;

    // Record commands
    context->reset();
    context->begin();
    context->begin_render_pass(render_pass.get(), framebuffers[image_index].get(), {0.1f, 0.1f, 0.1f, 1.0f});
    context->bind_pipeline(pipeline.get());
    context->set_viewport(0, 0, 1280, 720, 0.0f, 1.0f);
    context->set_scissor(0, 0, 1280, 720);
    context->draw(3, 1, 0, 0);  // Draw triangle
    context->end_render_pass();
    context->end();

    // Submit
    queue->submit({
        .wait_semaphores = {image_available_sems[frame_index].get()},
        .command_contexts = {context.get()},
        .signal_semaphores = {render_finished_sems[frame_index].get()},
        .signal_fence = fences[frame_index].get(),
    });

    // Present
    device->present({render_finished_sems[frame_index].get()});

    frame_index = (frame_index + 1) % MAX_FRAMES;
}
```

---

## Intermediate Tutorial: Rendering with Textures

This tutorial builds on the basics to add textured meshes with vertex buffers.

*(Coming soon)*

---

## Advanced Tutorial: Bindless Rendering

This tutorial demonstrates Sapfire's cross-platform bindless rendering system, which uses a unified shader approach that works on both DirectX 12 and Vulkan.

### Overview

Bindless rendering eliminates the need to bind individual resources before draw calls. Instead:
1. All resources (buffers, textures) are registered in a global descriptor set
2. Shaders access resources by index through push constants
3. One draw call can reference any combination of resources

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  BindlessResourceRegistry                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Set 0, Binding 0: StorageBuffer[] (all buffers)     │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Set 1, Binding 0: SampledImage[] (all textures)     │   │
│  │ Set 1, Binding 1: Sampler (point clamp sampler)     │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              ↑
        register_buffer()     │    register_texture()
        returns index         │    returns index
                              │
┌─────────────────────────────┴───────────────────────────────┐
│                    Per-Draw Push Constants                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ pos_buffer_index, normal_buffer_index,              │   │
│  │ uv_buffer_index, scene_data_buffer_index,           │   │
│  │ pass_data_buffer_index, material_data_buffer_index, │   │
│  │ texture_data_buffer_index                           │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Cross-Platform Shader System

Sapfire uses a **unified shader approach** with platform-specific macros. One HLSL source file compiles to both D3D12 (DXIL) and Vulkan (SPIR-V).

#### Shader File Structure

```
assets/shaders/
├── common.hlsl           # Shared structs and includes bindless_defines.hlsl
├── bindless_defines.hlsl # Platform macros (D3D12 vs Vulkan)
├── bindless_rs.hlsl      # D3D12 root signature (D3D12 only)
├── bindless.hlsl         # Main shader using macros
└── lighting_util.hlsl    # Lighting functions
```

#### Step 1: Define Data Structures (common.hlsl)

All shared data structures must be defined in \`common.hlsl\` **before** including \`bindless_defines.hlsl\`:

```hlsl
// common.hlsl
#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#pragma pack_matrix(row_major)

#include "lighting_util.hlsl"

// Type Aliases
typedef int s32;
typedef uint u32;

// Per-draw constants passed via push constants (32 bytes)
struct PerDrawConstants
{
    u32 pos_buffer_index;       // Index of position buffer in bindless array
    u32 normal_buffer_index;    // Index of normal buffer in bindless array
    u32 tangent_buffer_index;   // Index of tangent buffer in bindless array
    u32 uv_buffer_index;        // Index of UV buffer in bindless array
    u32 scene_data_buffer_index;    // Index of SceneData buffer
    u32 pass_data_buffer_index;     // Index of PassData buffer
    u32 material_data_buffer_index; // Index of MaterialData buffer
    u32 texture_data_buffer_index;  // Index of albedo texture
};

// Per-object transformation data
struct SceneData
{
    float4x4 world;  // World matrix
};

// Per-frame camera and lighting data
struct PassData
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

// Material properties
struct MaterialData
{
    float4 gAlbedo;
    float3 gFresnel;
    float gRoughness;
};

// Include bindless macros AFTER struct definitions
#include "bindless_defines.hlsl"

#endif // __COMMON_HLSLI__
```

#### Step 2: Platform Macros (bindless_defines.hlsl)

This file contains all platform-specific declarations. The key insight is that Vulkan uses explicit unbounded arrays with \`[[vk::binding]]\` attributes, while D3D12 uses \`ResourceDescriptorHeap\`.

```hlsl
// bindless_defines.hlsl
#ifndef __BINDLESS_DEFINES_HLSLI__
#define __BINDLESS_DEFINES_HLSLI__

#ifdef VULKAN
    //=========================================
    // VULKAN: Push constants + explicit unbounded typed arrays
    //=========================================

    // Push constants for per-draw indices (no descriptor slot needed)
    #define DECLARE_PUSH_CONSTANTS [[vk::push_constant]] ConstantBuffer<PerDrawConstants> renderResources

    // Bindless buffer arrays - Set 0, Binding 0
    // Multiple typed arrays alias the same binding (different views of same descriptor array)
    [[vk::binding(0, 0)]] StructuredBuffer<float3> g_Float3Buffers[] : register(t0, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<float2> g_Float2Buffers[] : register(t1, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<SceneData> g_SceneDataBuffers[] : register(t2, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<PassData> g_PassDataBuffers[] : register(t3, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<MaterialData> g_MaterialDataBuffers[] : register(t4, space0);

    // Bindless textures - Set 1, Binding 0
    [[vk::binding(0, 1)]] Texture2D<float4> g_Textures[] : register(t0, space1);

    // Sampler - Set 1, Binding 1
    [[vk::binding(1, 1)]] SamplerState g_Sampler : register(s0);

    // Buffer access macros - fully typed, no Load() helpers needed
    #define GET_FLOAT3_BUFFER(index) g_Float3Buffers[NonUniformResourceIndex(index)]
    #define GET_FLOAT2_BUFFER(index) g_Float2Buffers[NonUniformResourceIndex(index)]
    #define GET_SCENE_DATA(index) g_SceneDataBuffers[NonUniformResourceIndex(index)][0]
    #define GET_PASS_DATA(index) g_PassDataBuffers[NonUniformResourceIndex(index)][0]
    #define GET_MATERIAL_DATA(index) g_MaterialDataBuffers[NonUniformResourceIndex(index)][0]
    #define GET_TEXTURE(index) g_Textures[NonUniformResourceIndex(index)]
    #define GET_SAMPLER g_Sampler

#else
    //=========================================
    // D3D12: Root signature + ResourceDescriptorHeap
    //=========================================

    #define DECLARE_PUSH_CONSTANTS ConstantBuffer<PerDrawConstants> renderResources : register(b0)

    // D3D12 uses ResourceDescriptorHeap directly with casting
    #define GET_FLOAT3_BUFFER(index) ((StructuredBuffer<float3>)ResourceDescriptorHeap[index])
    #define GET_FLOAT2_BUFFER(index) ((StructuredBuffer<float2>)ResourceDescriptorHeap[index])
    #define GET_SCENE_DATA(index) ((ConstantBuffer<SceneData>)ResourceDescriptorHeap[index])
    #define GET_PASS_DATA(index) ((ConstantBuffer<PassData>)ResourceDescriptorHeap[index])
    #define GET_MATERIAL_DATA(index) ((ConstantBuffer<MaterialData>)ResourceDescriptorHeap[index])
    #define GET_TEXTURE(index) ((Texture2D<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(index)])
    #define GET_SAMPLER pointClampSampler

#endif

#endif // __BINDLESS_DEFINES_HLSLI__
```

**Key Implementation Details:**

1. **Aliased Bindings (Vulkan)**: Multiple \`StructuredBuffer<T>[]\` declarations can share the same \`[[vk::binding(0, 0)]]\`. Vulkan treats these as different shader-side interpretations of the same descriptor array. All \`StructuredBuffer<T>\` types compile to SSBO (Storage Buffer).

2. **NonUniformResourceIndex**: Required for dynamic indexing to ensure correct GPU divergent behavior.

3. **Push Constants vs Root Constants**: Vulkan uses \`[[vk::push_constant]]\`, D3D12 uses root constants defined in the root signature.

4. **No ByteAddressBuffer**: By using typed \`StructuredBuffer<T>[]\` arrays, we avoid manual \`Load()\` helpers and get type-safe access.

#### Step 3: Main Shader (bindless.hlsl)

The main shader uses macros exclusively for resource access, making it platform-agnostic:

```hlsl
// bindless.hlsl
#include "lighting_util.hlsl"
#include "common.hlsl"

#ifndef VULKAN
#include "bindless_rs.hlsl"  // D3D12 root signature
#endif

// Declare push constants / root constants
DECLARE_PUSH_CONSTANTS;

struct VSOut {
    float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW: TANGENT;
    float2 uv      : UV0;
};

#ifndef VULKAN
[RootSignature(BindlessRootSignature)]
#endif
VSOut VS(uint vertex_id: SV_VertexID)
{
    // Typed buffer access via macros
    float3 pos = GET_FLOAT3_BUFFER(renderResources.pos_buffer_index)[vertex_id];
    float3 normal = GET_FLOAT3_BUFFER(renderResources.normal_buffer_index)[vertex_id];
    float2 uv = GET_FLOAT2_BUFFER(renderResources.uv_buffer_index)[vertex_id];

    // Typed struct access - clean on both platforms
    SceneData scene_data = GET_SCENE_DATA(renderResources.scene_data_buffer_index);
    PassData pass_data = GET_PASS_DATA(renderResources.pass_data_buffer_index);

    float4 posW = mul(scene_data.world, float4(pos, 1.0));

    VSOut output = (VSOut)0.0f;
    output.posW = posW.xyz;
    output.normalW = mul((float3x3)scene_data.world, normal);
    output.posH = mul(pass_data.gViewProj, posW);
    output.uv = uv;
    return output;
}

#ifndef VULKAN
[RootSignature(BindlessRootSignature)]
#endif
float4 PS(VSOut input) : SV_TARGET
{
    Texture2D<float4> albedo_texture = GET_TEXTURE(renderResources.texture_data_buffer_index);
    MaterialData material = GET_MATERIAL_DATA(renderResources.material_data_buffer_index);
    PassData pass_data = GET_PASS_DATA(renderResources.pass_data_buffer_index);

    input.normalW = normalize(input.normalW);
    float3 toEyeW = normalize(pass_data.gEyePosW - input.posW);

    float4 ambient = pass_data.gAmbientLight * material.gAlbedo;
    const float shininess = 1.0f - material.gRoughness;
    Material mat = { material.gAlbedo, material.gFresnel, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(pass_data.gLights, mat, input.posW,
        input.normalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    litColor.a = material.gAlbedo.a;

    return litColor * albedo_texture.Sample(GET_SAMPLER, input.uv);
}
```

#### Step 4: Shader Compilation

Compile the same source file for both platforms:

```bash
#!/bin/bash
# compile_shaders.sh

# Vulkan (SPIR-V) - Use SM 6.0, add -D VULKAN
dxc -spirv -T vs_6_0 -E VS -D VULKAN \
    -fspv-extension=SPV_EXT_descriptor_indexing \
    -fspv-target-env=vulkan1.2 \
    bindless.hlsl -Fo bindless-vulkan.vert.spv

dxc -spirv -T ps_6_0 -E PS -D VULKAN \
    -fspv-extension=SPV_EXT_descriptor_indexing \
    -fspv-target-env=vulkan1.2 \
    bindless.hlsl -Fo bindless-vulkan.frag.spv

# D3D12 (DXIL) - Use SM 6.6 for ResourceDescriptorHeap
dxc -T vs_6_6 -E VS bindless.hlsl -Fo bindless_vs.cso
dxc -T ps_6_6 -E PS bindless.hlsl -Fo bindless_ps.cso
```

**Compilation Flags Explained:**

| Flag | Purpose |
|------|---------|
| \`-D VULKAN\` | Enables Vulkan code path in bindless_defines.hlsl |
| \`-fspv-extension=SPV_EXT_descriptor_indexing\` | Required for unbounded arrays |
| \`-fspv-target-env=vulkan1.2\` | Target Vulkan 1.2 for descriptor indexing support |
| \`-T vs_6_0\` / \`-T ps_6_0\` | Shader Model 6.0 for Vulkan (avoid SM 6.6 SPIR-V issues) |
| \`-T vs_6_6\` / \`-T ps_6_6\` | Shader Model 6.6 for D3D12 ResourceDescriptorHeap |

#### Step 5: C++ Setup - BindlessResourceRegistry

```cpp
#include <render/bindless_resource_registry.h>

// Create descriptor pool with enough capacity
auto pool_result = device->create_descriptor_pool({
    .max_sets = 100,
    .max_storage_buffers = 10000,
    .max_sampled_images = 10000,
    .max_samplers = 16,
    .name = "Bindless Pool",
});
auto pool = std::move(*pool_result);

// Get default bindless layout (matches shader expectations)
auto layouts = BindlessResourceRegistry::default_descriptor_set_layout(
    10000,  // max_textures
    10000   // max_buffers
);

// Create the registry
BindlessResourceRegistry registry(pool.get(), layouts);

// Create and register a sampler
auto sampler_result = device->create_sampler({
    .min_filter = Filter::Linear,
    .mag_filter = Filter::Linear,
    .address_u = AddressMode::Repeat,
    .address_v = AddressMode::Repeat,
    .name = "Default Sampler",
});
registry.set_default_sampler(sampler_result->get());
```

#### Step 6: Register Resources

```cpp
// Create and register vertex buffers
auto pos_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,  // Must be Storage for bindless
    .size_in_bytes = positions.size() * sizeof(float3),
    .name = "Position Buffer",
});
pos_buffer->update(positions.data(), positions.size() * sizeof(float3));
u32 pos_index = registry.register_buffer(*pos_buffer);

auto normal_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,
    .size_in_bytes = normals.size() * sizeof(float3),
    .name = "Normal Buffer",
});
normal_buffer->update(normals.data(), normals.size() * sizeof(float3));
u32 normal_index = registry.register_buffer(*normal_buffer);

auto uv_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,
    .size_in_bytes = uvs.size() * sizeof(float2),
    .name = "UV Buffer",
});
uv_buffer->update(uvs.data(), uvs.size() * sizeof(float2));
u32 uv_index = registry.register_buffer(*uv_buffer);

// Create and register constant buffers
auto scene_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,  // Storage, not Uniform, for bindless
    .size_in_bytes = sizeof(SceneData),
    .name = "Scene Data",
});
u32 scene_index = registry.register_buffer(*scene_buffer);

auto pass_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,
    .size_in_bytes = sizeof(PassData),
    .name = "Pass Data",
});
u32 pass_index = registry.register_buffer(*pass_buffer);

auto material_buffer = allocator->allocate_buffer({
    .usage = BufferUsage::Storage,
    .size_in_bytes = sizeof(MaterialData),
    .name = "Material Data",
});
u32 material_index = registry.register_buffer(*material_buffer);

// Register texture
u32 texture_index = registry.register_texture(*albedo_texture, sampler.get());
```

#### Step 7: Update Per-Frame Data

```cpp
void update_buffers(float delta_time) {
    // Update pass constants
    m_PassConstants.gView = m_Camera.get_view_matrix();
    m_PassConstants.gProj = m_Camera.get_projection_matrix();
    m_PassConstants.gViewProj = m_PassConstants.gView * m_PassConstants.gProj;
    m_PassConstants.gEyePosW = m_Camera.get_position();
    m_PassConstants.gInvView = m_PassConstants.gView.inversed();
    m_PassConstants.gInvProj = m_PassConstants.gProj.inversed();
    m_PassConstants.gInvViewProj = m_PassConstants.gViewProj.inversed();
    m_PassConstants.gNearZ = m_Camera.near_plane;
    m_PassConstants.gFarZ = m_Camera.far_plane;
    m_PassConstants.gTotalTime += delta_time;
    m_PassConstants.gDeltaTime = delta_time;

    // Upload to GPU
    m_PassBuffer.update(&m_PassConstants, sizeof(PassConstants));
}
```

#### Step 8: Render Loop with Push Constants

```cpp
void render() {
    context->reset();
    context->begin();

    // Bind descriptor sets (only once per frame)
    context->bind_descriptor_sets(pipeline_layout.get(), {
        registry.get_descriptor_set(0),  // Buffers
        registry.get_descriptor_set(1),  // Textures + Sampler
    });

    context->begin_render_pass(render_pass.get(), framebuffer.get(), clear_color);
    context->bind_pipeline(pipeline.get());
    context->set_viewport(0, 0, width, height, 0.0f, 1.0f);
    context->set_scissor(0, 0, width, height);

    // For each object
    for (const auto& object : scene_objects) {
        // Update object's world matrix
        SceneData scene_data{ .world = object.transform.matrix() };
        object.scene_buffer->update(&scene_data, sizeof(SceneData));

        // Set per-draw push constants (indices into bindless arrays)
        PerDrawConstants draw_constants{
            .pos_buffer_index = object.pos_buffer_index,
            .normal_buffer_index = object.normal_buffer_index,
            .tangent_buffer_index = object.tangent_buffer_index,
            .uv_buffer_index = object.uv_buffer_index,
            .scene_data_buffer_index = object.scene_data_index,
            .pass_data_buffer_index = m_PassDataIndex,
            .material_data_buffer_index = object.material_index,
            .texture_data_buffer_index = object.texture_index,
        };
        context->push_constants(pipeline_layout.get(), ShaderStage::AllGraphics,
                               0, sizeof(PerDrawConstants), &draw_constants);

        // Draw (vertex pulling - no vertex buffer binding needed)
        context->draw(object.vertex_count, 1, 0, 0);
    }

    context->end_render_pass();
    context->end();

    queue->submit({...});
}
```

### BindlessResourceRegistry API Reference

```cpp
class BindlessResourceRegistry {
public:
    // Create default descriptor set layouts matching shader expectations
    // Returns layouts for Set 0 (buffers) and Set 1 (textures + sampler)
    static stl::vector<DescriptorSetLayout> default_descriptor_set_layout(
        u32 max_textures,  // Maximum number of textures
        u32 max_buffers    // Maximum number of buffers
    );

    // Construct registry with pre-allocated descriptor sets
    BindlessResourceRegistry(
        IDescriptorPool* pool,
        stl::span<const DescriptorSetLayout> layouts
    );

    // Register a buffer, returns index for shader access
    u32 register_buffer(Buffer& buffer);

    // Register a texture with sampler, returns index for shader access
    u32 register_texture(Texture& texture, ISampler* sampler);

    // Set the default sampler used in shaders
    void set_default_sampler(ISampler* sampler);

    // Unregister resources (index can be reused)
    void unregister_buffer(u32 index);
    void unregister_texture(u32 index);

    // Get descriptor set for binding
    IDescriptorSet* get_descriptor_set(u32 set_index) const;
};
```

### Descriptor Layout Reference

The default bindless layout created by \`default_descriptor_set_layout()\`:

| Set | Binding | Type | Description |
|-----|---------|------|-------------|
| 0 | 0 | StorageBuffer[] | All registered buffers (variable count) |
| 1 | 0 | SampledImage[] | All registered textures (fixed count) |
| 1 | 1 | Sampler | Default sampler |

**Important**: All buffers must use \`BufferUsage::Storage\` to be compatible with the \`StorageBuffer\` descriptor type used in the bindless registry.

---

## API Reference

*(Coming soon)*

---

## Best Practices

### Bindless Rendering

1. **Use Storage Buffers**: All buffers in bindless must use \`BufferUsage::Storage\`, not \`Uniform\`.

2. **NonUniformResourceIndex**: Always wrap dynamic indices with \`NonUniformResourceIndex()\` in shaders.

3. **Typed Arrays over ByteAddressBuffer**: Use \`StructuredBuffer<T>[]\` for type safety instead of raw byte access.

4. **Aliased Bindings**: Multiple typed array declarations can share the same Vulkan binding - they're different views of the same descriptor array.

5. **Push Constants for Indices**: Keep per-draw data small (indices only) in push constants for efficiency.

### Memory Management

1. **Always use MemTags**: All STL containers require explicit memory tags.

2. **Persistent vs Temporary**: Use \`mem::MemTag::Render\` for persistent render resources, \`mem::MemTag::Temp\` for per-frame data.

### Synchronization

1. **Double/Triple Buffering**: Use multiple fences and semaphores for parallel CPU/GPU work.

2. **Wait Before Reuse**: Always wait on a fence before reusing its associated command context.

### Shader Compilation

1. **Use DXC**: The DirectX Shader Compiler (DXC) supports both DXIL and SPIR-V output.

2. **SM 6.0 for Vulkan**: Use Shader Model 6.0 for Vulkan SPIR-V to avoid ResourceDescriptorHeap issues.

3. **SM 6.6 for D3D12**: Use Shader Model 6.6 for D3D12 to get ResourceDescriptorHeap support.

