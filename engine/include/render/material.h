#pragma once

#include "math/math.h"
#include "core/core.h"
#include "render/render_compat.h"

namespace Sapfire::d3d {

	struct SFAPI MaterialConstants {
		sf::math::vec4 diffuse_albedo{1.f, 1.f, 1.f, 1.f};
		sf::math::vec3 fresnel_r0{0.01f, 0.01f, 0.01f};
		f32 roughness = 0.25f;
	};

	struct SFAPI Material {
		stl::string name;
		sf::math::vec4 diffuse_albedo{1.f, 1.f, 1.f, 1.f};
		sf::math::vec3 fresnel_r0{0.01f, 0.01f, 0.01f};
		f32 roughness = 0.25f;
		Buffer material_buffer{};
		s32 material_cb_index = -1;
	};
} // namespace Sapfire::d3d
