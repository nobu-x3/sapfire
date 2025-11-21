#pragma once

#include "core/core.h"
#include <cmath>

namespace sf::math {

struct vec2 {
    f32 x, y;

    // Constructors
    constexpr vec2() : x(0.0f), y(0.0f) {}
    constexpr vec2(f32 x, f32 y) : x(x), y(y) {}
    constexpr explicit vec2(f32 scalar) : x(scalar), y(scalar) {}

    // Array access
    f32& operator[](size_t i) { return (&x)[i]; }
    const f32& operator[](size_t i) const { return (&x)[i]; }

    // Arithmetic operators
    vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator*(f32 s) const { return vec2(x * s, y * s); }
    vec2 operator/(f32 s) const { return vec2(x / s, y / s); }

    vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
    vec2& operator/=(f32 s) { x /= s; y /= s; return *this; }

    // Comparison
    bool operator==(const vec2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const vec2& v) const { return !(*this == v); }

    // Unary operators
    vec2 operator-() const { return vec2(-x, -y); }

    // Math functions
    f32 length() const { return std::sqrt(x * x + y * y); }
    f32 length_squared() const { return x * x + y * y; }

    vec2 normalized() const {
        f32 len = length();
        return len > 0.0f ? (*this / len) : vec2(0.0f);
    }

    void normalize() {
        f32 len = length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
        }
    }

    // Dot product
    f32 dot(const vec2& v) const { return x * v.x + y * v.y; }

    // Static functions
    static f32 dot(const vec2& a, const vec2& b) { return a.dot(b); }
    static vec2 lerp(const vec2& a, const vec2& b, f32 t) {
        return a + (b - a) * t;
    }
};

// Free functions
inline vec2 operator*(f32 s, const vec2& v) { return v * s; }

} // namespace sf::math
