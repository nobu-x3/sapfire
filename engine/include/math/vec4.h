#pragma once

#include "core/core.h"
#include "vec3.h"
#include <cmath>

namespace sf::math {

struct vec4 {
    f32 x, y, z, w;

    // Constructors
    constexpr vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    constexpr explicit vec4(f32 scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    constexpr vec4(const vec3& v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) {}

    // Array access
    f32& operator[](size_t i) { return (&x)[i]; }
    const f32& operator[](size_t i) const { return (&x)[i]; }

    // Arithmetic operators
    vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator-(const vec4& v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator*(f32 s) const { return vec4(x * s, y * s, z * s, w * s); }
    vec4 operator/(f32 s) const { return vec4(x / s, y / s, z / s, w / s); }

    // Component-wise multiplication
    vec4 operator*(const vec4& v) const { return vec4(x * v.x, y * v.y, z * v.z, w * v.w); }

    vec4& operator+=(const vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    vec4& operator-=(const vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    vec4& operator*=(f32 s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    vec4& operator/=(f32 s) { x /= s; y /= s; z /= s; w /= s; return *this; }

    // Comparison
    bool operator==(const vec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const vec4& v) const { return !(*this == v); }

    // Unary operators
    vec4 operator-() const { return vec4(-x, -y, -z, -w); }

    // Math functions
    f32 length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    f32 length_squared() const { return x * x + y * y + z * z + w * w; }

    vec4 normalized() const {
        f32 len = length();
        return len > 0.0f ? (*this / len) : vec4(0.0f);
    }

    void normalize() {
        f32 len = length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
        }
    }

    // Dot product
    f32 dot(const vec4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }

    // Convert to vec3 (drop w)
    vec3 xyz() const { return vec3(x, y, z); }

    // Static functions
    static f32 dot(const vec4& a, const vec4& b) { return a.dot(b); }
    static vec4 lerp(const vec4& a, const vec4& b, f32 t) {
        return a + (b - a) * t;
    }
};

// Free functions
inline vec4 operator*(f32 s, const vec4& v) { return v * s; }

} // namespace sf::math
