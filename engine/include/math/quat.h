#pragma once
#include "core/core.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace sf::math {

struct quat {
    f32 x, y, z, w;

    // Constructors
    constexpr quat() : x(0), y(0), z(0), w(1) {}
    constexpr quat(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    constexpr quat(const vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

    // Identity quaternion
    static constexpr quat identity() {
        return quat(0, 0, 0, 1);
    }

    // Create from Euler angles (roll, pitch, yaw)
    static quat from_euler(f32 roll, f32 pitch, f32 yaw);
    static quat from_euler(const vec3& euler);
    static quat from_euler(const vec4& euler);

    // Create from axis-angle
    static quat from_axis_angle(const vec3& axis, f32 angle);

    // Quaternion multiplication
    quat operator*(const quat& q) const;

    // Conjugate
    quat conjugate() const {
        return quat(-x, -y, -z, w);
    }

    // Length
    f32 length() const;
    f32 length_squared() const {
        return x * x + y * y + z * z + w * w;
    }

    // Normalize
    quat normalized() const;
    void normalize();

    // Dot product
    f32 dot(const quat& q) const {
        return x * q.x + y * q.y + z * q.z + w * q.w;
    }

    static f32 dot(const quat& a, const quat& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    // Spherical linear interpolation
    static quat slerp(const quat& q1, const quat& q2, f32 t);

    // Rotate a vector by this quaternion
    vec3 rotate(const vec3& v) const;
    vec4 rotate(const vec4& v) const;

    // Convert to vec4
    vec4 to_vec4() const {
        return vec4(x, y, z, w);
    }
};

} // namespace sf::math
