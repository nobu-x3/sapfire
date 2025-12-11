#ifndef __BINDLESS_DEFINES_HLSLI__
#define __BINDLESS_DEFINES_HLSLI__

#ifdef VULKAN
    //=========================================
    // VULKAN: Push constants + explicit unbounded typed arrays
    //=========================================

    // Push constants for per-draw indices (no descriptor needed)
    #define DECLARE_PUSH_CONSTANTS [[vk::push_constant]] ConstantBuffer<PerDrawConstants> renderResources

    // Bindless buffer arrays - Set 0, Binding 0
    // One array per type used in the shader (all aliased at same binding)
    [[vk::binding(0, 0)]] StructuredBuffer<float3> g_Float3Buffers[] : register(t0, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<float2> g_Float2Buffers[] : register(t1, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<SceneData> g_SceneDataBuffers[] : register(t2, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<PassData> g_PassDataBuffers[] : register(t3, space0);
    [[vk::binding(0, 0)]] StructuredBuffer<MaterialData> g_MaterialDataBuffers[] : register(t4, space0);

    // Bindless textures - Set 1, Binding 0
    [[vk::binding(0, 1)]] Texture2D<float4> g_Textures[] : register(t0, space1);

    // Sampler - Set 1, Binding 1
    [[vk::binding(1, 1)]] SamplerState g_Sampler : register(s0);

    // Buffer access macros - fully typed, no Load() needed!
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

    // D3D12 uses ResourceDescriptorHeap directly - cast to appropriate type
    #define GET_FLOAT3_BUFFER(index) ((StructuredBuffer<float3>)ResourceDescriptorHeap[index])
    #define GET_FLOAT2_BUFFER(index) ((StructuredBuffer<float2>)ResourceDescriptorHeap[index])
    #define GET_SCENE_DATA(index) ((ConstantBuffer<SceneData>)ResourceDescriptorHeap[index])
    #define GET_PASS_DATA(index) ((ConstantBuffer<PassData>)ResourceDescriptorHeap[index])
    #define GET_MATERIAL_DATA(index) ((ConstantBuffer<MaterialData>)ResourceDescriptorHeap[index])
    #define GET_TEXTURE(index) ((Texture2D<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(index)])
    #define GET_SAMPLER pointClampSampler

#endif

#endif // __BINDLESS_DEFINES_HLSLI__
