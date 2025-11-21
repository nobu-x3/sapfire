#include "engpch.h"

#include "tools/texture_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace sf::tools::texture_loader {

	void* load(const char* path, i32& width, i32& height, i32 compontent_count) {
		void* data = stbi_load(path, &width, &height, nullptr, compontent_count);
		if (!data) {
            const char* reason = stbi_failure_reason();
			CORE_ERROR("Error loading texture at path {}: {}", path, reason);
		}
		return data;
	}

    f32* load_hdr(const char* path, i32& width, i32& height, i32 component_count){
        f32* data = stbi_loadf(path, &width, &height, nullptr, component_count);
		if (!data) {
            const char* reason = stbi_failure_reason();
			CORE_ERROR("Error loading texture at path {}: {}", path, reason);
		}
		return data;
    }
} // namespace sf::tools::texture_loader
