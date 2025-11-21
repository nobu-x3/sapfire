#pragma once

namespace sf::tools::texture_loader {
    void* load(const char* path, i32& width, i32& height, i32 compontent_count);
    f32* load_hdr(const char* path, i32& width, i32& height, i32 component_count);
}
