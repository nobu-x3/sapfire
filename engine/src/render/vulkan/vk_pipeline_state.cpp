#include "render/vulkan/vk_pipeline_state.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_shader_compiler.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {
    VkPipelineState::~VkPipelineState() {
        if (m_Pipeline != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
        }
    }

    stl::result<> VkPipelineState::create_graphics(VkDevice device, const GraphicsPipelineStateDesc& desc, VkPipelineLayout layout,
                                                   VkRenderPass render_pass) {
        m_Device = device;
        m_IsCompute = false;
        // Load and compile shaders
        Shader vertex_shader =
            compile(device, ShaderType::Vertex, desc.shader_module.vertex_shader_path, desc.shader_module.vertex_entry_point);
        Shader fragment_shader =
            compile(device, ShaderType::Fragment, desc.shader_module.pixel_shader_path, desc.shader_module.pixel_entry_point);
        if (vertex_shader.module == VK_NULL_HANDLE || fragment_shader.module == VK_NULL_HANDLE) {
            destroy_shader_module(device, vertex_shader);
            destroy_shader_module(device, fragment_shader);
            return stl::make_error<>("Failed to load shaders for graphics pipeline");
        }
        // Shader stages - use entry points from descriptor
        // Note: string_view data() returns a pointer that remains valid as long as the original string exists
        VkPipelineShaderStageCreateInfo vertex_stage{};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader.module;
        vertex_stage.pName = desc.shader_module.vertex_entry_point.data(); // Entry point from descriptor
        VkPipelineShaderStageCreateInfo fragment_stage{};
        fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_stage.module = fragment_shader.module;
        fragment_stage.pName = desc.shader_module.pixel_entry_point.data(); // Entry point from descriptor
        VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage, fragment_stage};
        // Vertex input state (for now, assume no vertex input - bindless rendering)
        VkPipelineVertexInputStateCreateInfo vertex_input{};
        vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount = 0;
        vertex_input.vertexAttributeDescriptionCount = 0;
        // Input assembly state
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = to_vk_primitive_topology(desc.primitive_topology);
        input_assembly.primitiveRestartEnable = VK_FALSE;
        // Viewport state (will be set dynamically)
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;
        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = to_vk_polygon_mode(desc.rasterizer.fill_mode);
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = to_vk_cull_mode(desc.rasterizer.cull_mode);
        rasterizer.frontFace = desc.rasterizer.front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = desc.rasterizer.depth_bias != 0;
        rasterizer.depthBiasConstantFactor = static_cast<float>(desc.rasterizer.depth_bias);
        rasterizer.depthBiasClamp = desc.rasterizer.depth_bias_clamp;
        rasterizer.depthBiasSlopeFactor = desc.rasterizer.slope_scaled_depth_bias;
        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        // Depth-stencil state
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = desc.depth_stencil.depth_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthWriteEnable = desc.depth_stencil.depth_write_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthCompareOp = to_vk_compare_op(desc.depth_stencil.depth_func);
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = desc.depth_stencil.stencil_enable ? VK_TRUE : VK_FALSE;
        // Color blend attachments
        stl::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{mem::MemTag::Temp};
        for (u32 i = 0; i < desc.rtv_count && i < desc.blend_states.size(); ++i) {
            const auto& blend = desc.blend_states[i];
            VkPipelineColorBlendAttachmentState attachment{};
            attachment.blendEnable = blend.blend_enable ? VK_TRUE : VK_FALSE;
            attachment.srcColorBlendFactor = to_vk_blend_factor(blend.src_blend);
            attachment.dstColorBlendFactor = to_vk_blend_factor(blend.dst_blend);
            attachment.colorBlendOp = to_vk_blend_op(blend.blend_op);
            attachment.srcAlphaBlendFactor = to_vk_blend_factor(blend.src_blend_alpha);
            attachment.dstAlphaBlendFactor = to_vk_blend_factor(blend.dst_blend_alpha);
            attachment.alphaBlendOp = to_vk_blend_op(blend.blend_op_alpha);
            attachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachments.push_back(attachment);
        }
        // Color blend state
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = static_cast<u32>(color_blend_attachments.size());
        color_blending.pAttachments = color_blend_attachments.data();
        // Dynamic state
        VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = 2;
        dynamic_state.pDynamicStates = dynamic_states;
        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shader_stages;
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = 0;
        VK_RETURN_ON_ERROR(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_Pipeline),
                           "Failed to create graphics pipeline");

        CORE_INFO("Created graphics pipeline");

        // Clean up shader modules
        destroy_shader_module(device, vertex_shader);
        destroy_shader_module(device, fragment_shader);

        return stl::result_success();
    }

    stl::result<> VkPipelineState::create_compute(VkDevice device, const ComputePipelineStateDesc& desc, VkPipelineLayout layout) {
        m_Device = device;
        m_IsCompute = true;
        // Load and compile compute shader
        Shader compute_shader = compile(device, ShaderType::Compute, desc.shader_path, desc.entry_point);
        if (compute_shader.module == VK_NULL_HANDLE) {
            return stl::make_error<>("Failed to load compute shader");
        }
        // Shader stage
        VkPipelineShaderStageCreateInfo shader_stage{};
        shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shader_stage.module = compute_shader.module;
        shader_stage.pName = desc.entry_point.data(); // Entry point from descriptor
        // Create compute pipeline
        VkComputePipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.stage = shader_stage;
        pipeline_info.layout = layout;
        VK_RETURN_ON_ERROR(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_Pipeline),
                           "Failed to create compute pipeline");

        CORE_INFO("Created compute pipeline");

        // Clean up shader module
        destroy_shader_module(device, compute_shader);

        return stl::result_success();
    }
} // namespace sf::render::vk
