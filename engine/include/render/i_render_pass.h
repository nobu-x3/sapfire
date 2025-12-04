#pragma once

#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    // Load/Store Operations

    enum class LoadOp : u8 {
        Load, // Preserve existing contents
        Clear, // Clear to specified value
        DontCare // Don't care about previous contents
    };

    enum class StoreOp : u8 {
        Store, // Write results to memory
        DontCare // Don't need results after rendering
    };

    // Render Pass Description

    struct RenderPassDesc {
        struct AttachmentDesc {
            Format format = Format::Unknown;
            LoadOp load_op = LoadOp::DontCare;
            StoreOp store_op = StoreOp::Store;
            LoadOp stencil_load_op = LoadOp::DontCare;
            StoreOp stencil_store_op = StoreOp::DontCare;
            ResourceState initial_layout = ResourceState::Undefined;
            ResourceState final_layout = ResourceState::Present;
        };

        stl::vector<AttachmentDesc> color_attachments{mem::MemTag::Temp};
        stl::optional<AttachmentDesc> depth_attachment;
        const char* name = "Render Pass";
    };

    // Render Pass Interface

    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        virtual const RenderPassDesc& get_desc() const = 0;
        virtual void* get_native_render_pass() = 0;
    };

} // namespace sf::render
