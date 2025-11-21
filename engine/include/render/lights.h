#pragma once

#include "math/math.h"

namespace Sapfire::d3d {

	struct Light {
		sf::math::vec3 strength = {0.5f, 0.5f, 0.5f};
		float FalloffStart = 1.0f; // point/spot light only
		sf::math::vec3 direction = {0.0f, -1.0f, 0.0f}; // directional/spot light only
		float FalloffEnd = 10.0f; // point/spot light only
		sf::math::vec3 Position = {0.0f, 0.0f, 0.0f}; // point/spot light only
		float SpotPower = 64.0f; // spot light only
	};

#define MaxLights 16

} // namespace Sapfire::d3d
