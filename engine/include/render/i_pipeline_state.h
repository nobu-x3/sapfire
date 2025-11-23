#pragma once

#include "core/core.h"

namespace sf::render {

// Pipeline State Interface

class IPipelineState {
public:
    virtual ~IPipelineState() = default;

    // Query pipeline type
    virtual bool is_compute() const = 0;
    virtual bool is_graphics() const = 0;

    // Backend-specific handle
    virtual void* get_native_pipeline() = 0;
};

} // namespace sf::render
