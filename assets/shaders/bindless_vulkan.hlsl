// Cross-platform bindless shader example for Vulkan
// Uses Vulkan HLSL extensions with descriptor arrays

// ============================================================================
// Vulkan-specific bindings using descriptor arrays
// ============================================================================

// BindlessResourceRegistry layout:
// Set 0: Textures and Samplers
//   - Binding 0: Texture array (SampledImage) - fixed size array
//   - Binding 1: Sampler - single sampler
// Set 1: Buffers
//   - Binding 0: Buffer array (StorageBuffer) - variable count (can be last since it's the only binding in Set 1)

[[vk::binding(0, 0)]] Texture2D textures[] : register(t0, space0);
[[vk::binding(1, 0)]] SamplerState linearSampler : register(s0, space0);
[[vk::binding(0, 1)]] StructuredBuffer<float> buffers[] : register(t0, space1);

// Push constants for per-draw data
[[vk::push_constant]]
struct PushConstants {
    float4x4 modelMatrix;
    float4x4 viewProj;
    uint posBufferIndex;
    uint normalBufferIndex;
    uint uvBufferIndex;
    uint albedoTexIndex;
    uint materialBufferIndex;
    float3 cameraPos;
    float time;
} pushConstants;

// ============================================================================
// Data structures
// ============================================================================

struct MaterialData {
    float4 baseColor;
    float metallic;
    float roughness;
    float pad0, pad1;
};

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
    uint posBufferIdx = pushConstants.posBufferIndex;
    uint posOffset = vertexID * 3;
    float3 localPos = float3(
        buffers[posBufferIdx][posOffset],
        buffers[posBufferIdx][posOffset + 1],
        buffers[posBufferIdx][posOffset + 2]
    );

    uint normBufferIdx = pushConstants.normalBufferIndex;
    uint normOffset = vertexID * 3;
    float3 localNormal = float3(
        buffers[normBufferIdx][normOffset],
        buffers[normBufferIdx][normOffset + 1],
        buffers[normBufferIdx][normOffset + 2]
    );

    uint uvBufferIdx = pushConstants.uvBufferIndex;
    uint uvOffset = vertexID * 2;
    float2 uv = float2(
        buffers[uvBufferIdx][uvOffset],
        buffers[uvBufferIdx][uvOffset + 1]
    );

    // Transform to world space
    float4 worldPos = mul(pushConstants.modelMatrix, float4(localPos, 1.0));
    output.worldPos = worldPos.xyz;

    // Transform normal to world space (assuming uniform scale)
    output.normal = mul((float3x3)pushConstants.modelMatrix, localNormal);

    // Transform to clip space
    output.position = mul(pushConstants.viewProj, worldPos);
    output.uv = uv;

    return output;
}

// ============================================================================
// Pixel/Fragment Shader
// ============================================================================

float4 PS(VS_OUTPUT input) : SV_TARGET {
    // Get material data from bindless buffer
    uint matBufferIdx = pushConstants.materialBufferIndex;
    MaterialData material;
    material.baseColor = float4(
        buffers[matBufferIdx][0],
        buffers[matBufferIdx][1],
        buffers[matBufferIdx][2],
        buffers[matBufferIdx][3]
    );
    material.metallic = buffers[matBufferIdx][4];
    material.roughness = buffers[matBufferIdx][5];

    // Sample albedo texture using bindless texture array
    uint texIdx = pushConstants.albedoTexIndex;
    Texture2D albedoTex = textures[texIdx];

    float4 albedo = albedoTex.Sample(linearSampler, input.uv) * material.baseColor;

    // Simple lighting calculation
    float3 N = normalize(input.normal);
    float3 V = normalize(pushConstants.cameraPos - input.worldPos);

    // Basic ambient + simple directional light
    float3 lightDir = normalize(float3(0.5, -1.0, 0.3));
    float3 lightColor = float3(1.0, 1.0, 0.9);
    float3 ambient = float3(0.03, 0.03, 0.04);

    float diffuse = max(dot(N, -lightDir), 0.0);

    // Simple specular (Blinn-Phong)
    float3 H = normalize(-lightDir + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    float3 specular = spec * lightColor * material.metallic;

    float3 color = albedo.rgb * (ambient + lightColor * diffuse) + specular;

    return float4(color, albedo.a);
}
