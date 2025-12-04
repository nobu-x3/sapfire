#pragma once

#include <span>
#include "core/core.h"
#include "resource_types.h"

namespace sf::render {

    class IRenderPass;

    // Framebuffer Description

    struct FramebufferDesc {
        IRenderPass* render_pass = nullptr;
        stl::span<Texture*> color_attachments;
        Texture* depth_attachment = nullptr;
        u32 width = 0;
        u32 height = 0;
        const char* name = "Framebuffer";
    };

    // Framebuffer Interface

    class IFramebuffer {
    public:
        virtual ~IFramebuffer() = default;

        virtual u32 get_width() const = 0;
        virtual u32 get_height() const = 0;
        virtual void* get_native_framebuffer() = 0;
    };

} // namespace sf::render
