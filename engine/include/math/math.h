#pragma once

// Sapfire Math Library - Cross-platform replacement for DirectXMath

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "quat.h"
#include "mat4.h"
#include "aabb.h"
#include "frustum.h"

namespace sf::math {

// Constants
constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 TWO_PI = 2.0f * PI;
constexpr f32 HALF_PI = 0.5f * PI;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

// Utility functions
inline f32 to_radians(f32 degrees) { return degrees * DEG_TO_RAD; }
inline f32 to_degrees(f32 radians) { return radians * RAD_TO_DEG; }

inline f32 clamp(f32 value, f32 min, f32 max) {
    return value < min ? min : (value > max ? max : value);
}

inline f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

} // namespace sf::math
