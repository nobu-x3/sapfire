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

struct PerDrawConstants
{
    u32 pos_buffer_index;
    u32 normal_buffer_index;
    u32 tangent_buffer_index;
    u32 uv_buffer_index;
    u32 scene_data_buffer_index;
    u32 pass_data_buffer_index;
    u32 material_data_buffer_index;
    u32 texture_data_buffer_index;
};

struct SceneData
{
	float4x4 world;
};

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
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

struct MaterialData
{
    float4  gAlbedo;
    float3 gFresnel;
    float gRoughness;
};


// Include bindless macros AFTER struct definitions
#include "bindless_defines.hlsl"

#endif // __COMMON_HLSLI__
