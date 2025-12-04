#pragma once

#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    // Sampler Interface

    class ISampler {
    public:
        virtual ~ISampler() = default;

        virtual const SamplerDesc& get_desc() const = 0;
        virtual void* get_native_sampler() = 0;
    };

} // namespace sf::render
