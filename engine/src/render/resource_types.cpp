#include "render/resource_types.h"
#include <cstring>
#include "engpch.h"

namespace sf::render {

    void Buffer::update(const void* data, size_t size) {
        if (mapped_data && data && size <= size_in_bytes) {
            memcpy(mapped_data, data, size);
        }
    }
} // namespace sf::render
