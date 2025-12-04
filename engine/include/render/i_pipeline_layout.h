#pragma once

#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    // Descriptor Types

    enum class DescriptorType : u8 { UniformBuffer, StorageBuffer, CombinedImageSampler, SampledImage, StorageImage, Sampler };

    // Descriptor Set Layout

    struct DescriptorSetLayout {
        struct Binding {
            u32 binding = 0;
            DescriptorType type = DescriptorType::UniformBuffer;
            u32 count = 1;
            ShaderStage stages = ShaderStage::Vertex;
            bool variable_count = false; // For bindless arrays
        };

        stl::vector<Binding> bindings{mem::MemTag::Temp};
    };

    // Pipeline Layout Description

    struct PipelineLayoutDesc {
        struct PushConstantRange {
            ShaderStage stages = ShaderStage::Vertex;
            u32 offset = 0;
            u32 size = 0;
        };

        stl::vector<DescriptorSetLayout> descriptor_set_layouts{mem::MemTag::Temp};
        stl::vector<PushConstantRange> push_constant_ranges{mem::MemTag::Temp};
        const char* name = "Pipeline Layout";
    };

    // Pipeline Layout Interface

    class IPipelineLayout {
    public:
        virtual ~IPipelineLayout() = default;

        virtual const PipelineLayoutDesc& get_desc() const = 0;
        virtual void* get_native_layout() = 0;
    };

} // namespace sf::render
