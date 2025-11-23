// Cross-platform bindless shader example for Vulkan
// Uses Vulkan HLSL extensions with descriptor arrays

// ============================================================================
// Vulkan-specific bindings using descriptor arrays
// ============================================================================

// Set 0: Per-frame data
[[vk::binding(0, 0)]] cbuffer SceneData : register(b0, space0) {
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float3 cameraPos;
    float time;
};

[[vk::binding(1, 0)]] cbuffer PassData : register(b1, space0) {
    float4 ambientColor;
    float4 lightDirection;
    float4 lightColor;
};

// Set 1: Per-object data (push constants or descriptor)
[[vk::push_constant]]
struct PushConstants {
    uint posBufferIndex;
    uint normalBufferIndex;
    uint uvBufferIndex;
    uint materialIndex;
    float4x4 modelMatrix;
} pushConstants;

// Set 2: Bindless resource arrays (unbounded)
[[vk::binding(0, 2)]] StructuredBuffer<float3> positionBuffers[] : register(t0, space2);
[[vk::binding(1, 2)]] StructuredBuffer<float3> normalBuffers[] : register(t1, space2);
[[vk::binding(2, 2)]] StructuredBuffer<float2> uvBuffers[] : register(t2, space2);
[[vk::binding(3, 2)]] Texture2D textures[] : register(t3, space2);
[[vk::binding(4, 2)]] SamplerState samplers[] : register(s0, space2);

// Set 3: Material data
struct MaterialData {
    float4 baseColor;
    float metallic;
    float roughness;
    float pad0, pad1;
    uint albedoTexIndex;
    uint normalTexIndex;
    uint metallicTexIndex;
    uint roughnessTexIndex;
};

[[vk::binding(0, 3)]] StructuredBuffer<MaterialData> materials : register(t0, space3);

// ============================================================================
// Vertex Shader
// ============================================================================

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VS(uint vertexID : SV_VertexID) {
    VS_OUTPUT output;

    // Access bindless buffers using push constant indices
    StructuredBuffer<float3> positions = positionBuffers[pushConstants.posBufferIndex];
    StructuredBuffer<float3> normals = normalBuffers[pushConstants.normalBufferIndex];
    StructuredBuffer<float2> uvs = uvBuffers[pushConstants.uvBufferIndex];

    float3 localPos = positions[vertexID];
    float3 localNormal = normals[vertexID];
    float2 uv = uvs[vertexID];

    // Transform to world space
    float4 worldPos = mul(pushConstants.modelMatrix, float4(localPos, 1.0));
    output.worldPos = worldPos.xyz;

    // Transform normal to world space
    output.normal = mul((float3x3)pushConstants.modelMatrix, localNormal);

    // Transform to clip space
    output.position = mul(viewProj, worldPos);
    output.uv = uv;

    return output;
}

// ============================================================================
// Pixel/Fragment Shader
// ============================================================================

float4 PS(VS_OUTPUT input) : SV_TARGET {
    // Get material data
    MaterialData material = materials[pushConstants.materialIndex];

    // Sample albedo texture using bindless texture array
    // Use NonUniformResourceIndex for dynamic indexing
    Texture2D albedoTex = textures[NonUniformResourceIndex(material.albedoTexIndex)];
    SamplerState samp = samplers[0]; // Or use material.samplerIndex if needed

    float4 albedo = albedoTex.Sample(samp, input.uv) * material.baseColor;

    // Simple lighting
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDirection.xyz);
    float diffuse = max(dot(N, L), 0.0);

    float3 color = albedo.rgb * (ambientColor.rgb + lightColor.rgb * diffuse);

    return float4(color, albedo.a);
}
