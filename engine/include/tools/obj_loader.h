#pragma once

#include "render/d3d_primitives.h"

namespace sf::tools {
	class OBJLoader {
	public:
		static stl::optional<sf::render::primitives::MeshData> load_mesh(const stl::string& path);
	};
} // namespace sf::tools
