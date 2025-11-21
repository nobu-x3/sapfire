#pragma once

#include "core/core.h"
#include "math/math.h"

namespace Sapfire::d3d::primitives {

	struct Vertex {
		sf::math::vec3 position;
		sf::math::vec3 normal;
		sf::math::vec3 tangentu;
		sf::math::vec2 texc;

		Vertex() = default;
		Vertex(f32 px, f32 py, f32 pz, f32 nx, f32 ny, f32 nz, f32 tx, f32 ty, f32 tz, f32 u, f32 v) :
			position(px, py, pz), normal(nx, ny, nz), tangentu(tx, ty, tz), texc(u, v) {}
	};

	struct SFAPI MeshData {
		stl::vector<sf::math::vec3> positions;
		stl::vector<sf::math::vec3> normals;
		stl::vector<sf::math::vec3> tangentus;
		stl::vector<sf::math::vec2> texcs;
		stl::vector<u32> indices32;
		sf::math::aabb aabb;

		stl::vector<u16>& indices16() {
			if (m_Indices16.empty()) {
				m_Indices16.resize(indices32.size());
				for (size_t i = 0; i < indices32.size(); ++i)
					m_Indices16[i] = static_cast<u16>(indices32[i]);
			}
			return m_Indices16;
		}

	private:
		stl::vector<u16> m_Indices16;
	};

	MeshData create_box(float width, float height, float depth, u32 num_subdivisions);
	MeshData create_quad(float x, float y, float w, float h, float depth);

} // namespace Sapfire::d3d::primitives
