#include "math/quat.h"
#include <cmath>
#include "math/math.h"

namespace sf::math {
    quat quat::from_euler(f32 roll, f32 pitch, f32 yaw) {
        // Convert Euler angles to quaternion
        // Roll (X), Pitch (Y), Yaw (Z)
        f32 cy = std::cos(yaw * 0.5f);
        f32 sy = std::sin(yaw * 0.5f);
        f32 cp = std::cos(pitch * 0.5f);
        f32 sp = std::sin(pitch * 0.5f);
        f32 cr = std::cos(roll * 0.5f);
        f32 sr = std::sin(roll * 0.5f);
        quat q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;
        return q;
    }
    quat quat::from_euler(const vec3& euler) { return from_euler(euler.x, euler.y, euler.z); }
    quat quat::from_euler(const vec4& euler) { return from_euler(euler.x, euler.y, euler.z); }
    quat quat::from_axis_angle(const vec3& axis, f32 angle) {
        f32 half_angle = angle * 0.5f;
        f32 s = std::sin(half_angle);
        vec3 normalized_axis = axis.normalized();
        return quat(normalized_axis.x * s, normalized_axis.y * s, normalized_axis.z * s, std::cos(half_angle));
    }
    quat quat::operator*(const quat& q) const {
        return quat(w * q.x + x * q.w + y * q.z - z * q.y, w * q.y - x * q.z + y * q.w + z * q.x, w * q.z + x * q.y - y * q.x + z * q.w,
                    w * q.w - x * q.x - y * q.y - z * q.z);
    }
    f32 quat::length() const { return std::sqrt(length_squared()); }
    quat quat::normalized() const {
        f32 len = length();
        if (len > 0.0001f) {
            f32 inv_len = 1.0f / len;
            return quat(x * inv_len, y * inv_len, z * inv_len, w * inv_len);
        }
        return *this;
    }

    void quat::normalize() {
        f32 len = length();
        if (len > 0.0001f) {
            f32 inv_len = 1.0f / len;
            x *= inv_len;
            y *= inv_len;
            z *= inv_len;
            w *= inv_len;
        }
    }
    quat quat::slerp(const quat& q1, const quat& q2, f32 t) {
        quat result;
        // Compute dot product
        f32 dot_product = q1.dot(q2);
        // If the dot product is negative, slerp won't take the shorter path
        // Fix by reversing one quaternion
        quat q2_copy = q2;
        if (dot_product < 0.0f) {
            q2_copy = quat(-q2.x, -q2.y, -q2.z, -q2.w);
            dot_product = -dot_product;
        }
        // If quaternions are very close, use linear interpolation
        if (dot_product > 0.9995f) {
            result = quat(q1.x + t * (q2_copy.x - q1.x), q1.y + t * (q2_copy.y - q1.y), q1.z + t * (q2_copy.z - q1.z),
                          q1.w + t * (q2_copy.w - q1.w));
            result.normalize();
            return result;
        }
        // Clamp dot product to be in the domain of acos
        if (dot_product > 1.0f)
            dot_product = 1.0f;
        // Calculate coefficients
        f32 theta_0 = std::acos(dot_product);
        f32 theta = theta_0 * t;
        f32 sin_theta = std::sin(theta);
        f32 sin_theta_0 = std::sin(theta_0);
        f32 s0 = std::cos(theta) - dot_product * sin_theta / sin_theta_0;
        f32 s1 = sin_theta / sin_theta_0;
        return quat(s0 * q1.x + s1 * q2_copy.x, s0 * q1.y + s1 * q2_copy.y, s0 * q1.z + s1 * q2_copy.z, s0 * q1.w + s1 * q2_copy.w);
    }
    vec3 quat::rotate(const vec3& v) const {
        // Rodrigues rotation formula: v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
        vec3 q_xyz(x, y, z);
        vec3 t = vec3::cross(q_xyz, v) * 2.0f;
        return v + (t * w) + vec3::cross(q_xyz, t);
    }
    vec4 quat::rotate(const vec4& v) const {
        vec3 rotated = rotate(vec3(v.x, v.y, v.z));
        return vec4(rotated.x, rotated.y, rotated.z, v.w);
    }
} // namespace sf::math
