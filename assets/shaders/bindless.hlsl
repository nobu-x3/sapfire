#include "lighting_util.hlsl"
#include "common.hlsl"

#ifndef VULKAN
#include "bindless_rs.hlsl"
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

    // Typed struct access - clean on both platforms!
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
