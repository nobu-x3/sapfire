#include "math/mat4.h"
#include "math/quat.h"

namespace sf::math {
    mat4 mat4::from_quaternion(const quat& q) {
        mat4 result;
        f32 xx = q.x * q.x;
        f32 yy = q.y * q.y;
        f32 zz = q.z * q.z;
        f32 xy = q.x * q.y;
        f32 xz = q.x * q.z;
        f32 yz = q.y * q.z;
        f32 wx = q.w * q.x;
        f32 wy = q.w * q.y;
        f32 wz = q.w * q.z;
        result.m[0] = 1.0f - 2.0f * (yy + zz);
        result.m[1] = 2.0f * (xy + wz);
        result.m[2] = 2.0f * (xz - wy);
        result.m[3] = 0.0f;
        result.m[4] = 2.0f * (xy - wz);
        result.m[5] = 1.0f - 2.0f * (xx + zz);
        result.m[6] = 2.0f * (yz + wx);
        result.m[7] = 0.0f;
        result.m[8] = 2.0f * (xz + wy);
        result.m[9] = 2.0f * (yz - wx);
        result.m[10] = 1.0f - 2.0f * (xx + yy);
        result.m[11] = 0.0f;
        result.m[12] = 0.0f;
        result.m[13] = 0.0f;
        result.m[14] = 0.0f;
        result.m[15] = 1.0f;
        return result;
    }
    mat4 mat4::affine_transformation(const vec3& scale, const vec3& rotation_origin, const quat& rotation, const vec3& translation) {
        // Build the affine transformation matrix: M = Ms * Mro^-1 * Mr * Mro * Mt
        // Where Ms = scaling, Mro = rotation origin translation, Mr = rotation, Mt = translation
        // This matches DirectXMath's XMMatrixAffineTransformation
        mat4 result = mat4::scale(scale);
        // If rotation origin is not zero, apply: translate(-origin) * rotate * translate(origin)
        bool has_rotation_origin = (rotation_origin.x != 0.0f || rotation_origin.y != 0.0f || rotation_origin.z != 0.0f);
        if (has_rotation_origin) {
            result = result * mat4::translation(rotation_origin);
        }
        result = result * mat4::from_quaternion(rotation);
        if (has_rotation_origin) {
            result = result * mat4::translation(vec3(-rotation_origin.x, -rotation_origin.y, -rotation_origin.z));
        }
        result = result * mat4::translation(translation);
        return result;
    }
    mat4 mat4::affine_transformation(const vec4& scale, const vec4& rotation_origin, const quat& rotation, const vec4& translation) {
        return affine_transformation(vec3(scale.x, scale.y, scale.z), vec3(rotation_origin.x, rotation_origin.y, rotation_origin.z),
                                     rotation, vec3(translation.x, translation.y, translation.z));
    }
    f32 mat4::determinant() const {
        // Calculate determinant using the first row
        f32 det =
            m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - m[9] * (m[6] * m[15] - m[7] * m[14]) + m[13] * (m[6] * m[11] - m[7] * m[10])) -
            m[4] * (m[1] * (m[10] * m[15] - m[11] * m[14]) - m[9] * (m[2] * m[15] - m[3] * m[14]) + m[13] * (m[2] * m[11] - m[3] * m[10])) +
            m[8] * (m[1] * (m[6] * m[15] - m[7] * m[14]) - m[5] * (m[2] * m[15] - m[3] * m[14]) + m[13] * (m[2] * m[7] - m[3] * m[6])) -
            m[12] * (m[1] * (m[6] * m[11] - m[7] * m[10]) - m[5] * (m[2] * m[11] - m[3] * m[10]) + m[9] * (m[2] * m[7] - m[3] * m[6]));
        return det;
    }
} // namespace sf::math
