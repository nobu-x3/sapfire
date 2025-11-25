#pragma once

#include <cmath>
#include "core/core.h"

namespace sf::math {

    struct vec3 {
        f32 x, y, z;

        // Constructors
        constexpr vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        constexpr vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
        constexpr explicit vec3(f32 scalar) : x(scalar), y(scalar), z(scalar) {}

        // Array access
        f32& operator[](size_t i) { return (&x)[i]; }
        const f32& operator[](size_t i) const { return (&x)[i]; }

        // Arithmetic operators
        vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
        vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
        vec3 operator*(f32 s) const { return vec3(x * s, y * s, z * s); }
        vec3 operator/(f32 s) const { return vec3(x / s, y / s, z / s); }

        // Component-wise multiplication
        vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }

        vec3& operator+=(const vec3& v) {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }
        vec3& operator-=(const vec3& v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }
        vec3& operator*=(f32 s) {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }
        vec3& operator/=(f32 s) {
            x /= s;
            y /= s;
            z /= s;
            return *this;
        }

        // Comparison
        bool operator==(const vec3& v) const { return x == v.x && y == v.y && z == v.z; }
        bool operator!=(const vec3& v) const { return !(*this == v); }

        // Unary operators
        vec3 operator-() const { return vec3(-x, -y, -z); }

        // Math functions
        f32 length() const { return std::sqrt(x * x + y * y + z * z); }
        f32 length_squared() const { return x * x + y * y + z * z; }

        vec3 normalized() const {
            f32 len = length();
            return len > 0.0f ? (*this / len) : vec3(0.0f);
        }

        void normalize() {
            f32 len = length();
            if (len > 0.0f) {
                x /= len;
                y /= len;
                z /= len;
            }
        }

        // Dot product
        f32 dot(const vec3& v) const { return x * v.x + y * v.y + z * v.z; }

        // Cross product
        vec3 cross(const vec3& v) const { return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

        // Static functions
        static f32 dot(const vec3& a, const vec3& b) { return a.dot(b); }
        static vec3 cross(const vec3& a, const vec3& b) { return a.cross(b); }
        static vec3 lerp(const vec3& a, const vec3& b, f32 t) { return a + (b - a) * t; }
    };

    // Free functions
    inline vec3 operator*(f32 s, const vec3& v) { return v * s; }

} // namespace sf::math
