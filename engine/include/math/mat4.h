#pragma once

#include <cmath>
#include <cstring>
#include "core/core.h"
#include "vec3.h"
#include "vec4.h"

namespace sf::math {

    // Forward declarations
    struct quat;

    // Column-major 4x4 matrix
    struct mat4 {
        union {
            f32 m[16]; // Column-major: m[column * 4 + row]
            f32 m2[4][4]; // m2[column][row]
            vec4 columns[4];
        };

        // Constructors
        mat4() {
            // Identity matrix
            m[0] = 1.0f;
            m[4] = 0.0f;
            m[8] = 0.0f;
            m[12] = 0.0f;
            m[1] = 0.0f;
            m[5] = 1.0f;
            m[9] = 0.0f;
            m[13] = 0.0f;
            m[2] = 0.0f;
            m[6] = 0.0f;
            m[10] = 1.0f;
            m[14] = 0.0f;
            m[3] = 0.0f;
            m[7] = 0.0f;
            m[11] = 0.0f;
            m[15] = 1.0f;
        }

        mat4(f32 m00, f32 m01, f32 m02, f32 m03, f32 m10, f32 m11, f32 m12, f32 m13, f32 m20, f32 m21, f32 m22, f32 m23, f32 m30, f32 m31,
             f32 m32, f32 m33) {
            // Row-major constructor parameters, but store in column-major
            m[0] = m00;
            m[4] = m01;
            m[8] = m02;
            m[12] = m03;
            m[1] = m10;
            m[5] = m11;
            m[9] = m12;
            m[13] = m13;
            m[2] = m20;
            m[6] = m21;
            m[10] = m22;
            m[14] = m23;
            m[3] = m30;
            m[7] = m31;
            m[11] = m32;
            m[15] = m33;
        }

        // Access operators
        f32& operator[](size_t i) { return m[i]; }
        const f32& operator[](size_t i) const { return m[i]; }

        f32& operator()(size_t row, size_t col) { return m[col * 4 + row]; }
        const f32& operator()(size_t row, size_t col) const { return m[col * 4 + row]; }

        // Matrix multiplication
        mat4 operator*(const mat4& other) const {
            mat4 result;
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    f32 sum = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        sum += (*this)(row, k) * other(k, col);
                    }
                    result(row, col) = sum;
                }
            }
            return result;
        }

        mat4& operator*=(const mat4& other) {
            *this = *this * other;
            return *this;
        }

        // Vector multiplication
        vec4 operator*(const vec4& v) const {
            return vec4(m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w, m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
                        m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w, m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w);
        }

        vec3 transform_point(const vec3& v) const {
            vec4 result = (*this) * vec4(v, 1.0f);
            return result.xyz() / result.w;
        }

        vec3 transform_vector(const vec3& v) const {
            return vec3(m[0] * v.x + m[4] * v.y + m[8] * v.z, m[1] * v.x + m[5] * v.y + m[9] * v.z, m[2] * v.x + m[6] * v.y + m[10] * v.z);
        }

        // Transpose
        mat4 transposed() const {
            mat4 result;
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    result(row, col) = (*this)(col, row);
                }
            }
            return result;
        }

        void transpose() { *this = transposed(); }

        // Inverse (simplified - assumes non-degenerate matrix)
        mat4 inversed() const {
            mat4 inv;
            f32* inv_m = inv.m;
            const f32* mat = m;

            inv_m[0] = mat[5] * mat[10] * mat[15] - mat[5] * mat[11] * mat[14] - mat[9] * mat[6] * mat[15] + mat[9] * mat[7] * mat[14] +
                mat[13] * mat[6] * mat[11] - mat[13] * mat[7] * mat[10];
            inv_m[4] = -mat[4] * mat[10] * mat[15] + mat[4] * mat[11] * mat[14] + mat[8] * mat[6] * mat[15] - mat[8] * mat[7] * mat[14] -
                mat[12] * mat[6] * mat[11] + mat[12] * mat[7] * mat[10];
            inv_m[8] = mat[4] * mat[9] * mat[15] - mat[4] * mat[11] * mat[13] - mat[8] * mat[5] * mat[15] + mat[8] * mat[7] * mat[13] +
                mat[12] * mat[5] * mat[11] - mat[12] * mat[7] * mat[9];
            inv_m[12] = -mat[4] * mat[9] * mat[14] + mat[4] * mat[10] * mat[13] + mat[8] * mat[5] * mat[14] - mat[8] * mat[6] * mat[13] -
                mat[12] * mat[5] * mat[10] + mat[12] * mat[6] * mat[9];
            inv_m[1] = -mat[1] * mat[10] * mat[15] + mat[1] * mat[11] * mat[14] + mat[9] * mat[2] * mat[15] - mat[9] * mat[3] * mat[14] -
                mat[13] * mat[2] * mat[11] + mat[13] * mat[3] * mat[10];
            inv_m[5] = mat[0] * mat[10] * mat[15] - mat[0] * mat[11] * mat[14] - mat[8] * mat[2] * mat[15] + mat[8] * mat[3] * mat[14] +
                mat[12] * mat[2] * mat[11] - mat[12] * mat[3] * mat[10];
            inv_m[9] = -mat[0] * mat[9] * mat[15] + mat[0] * mat[11] * mat[13] + mat[8] * mat[1] * mat[15] - mat[8] * mat[3] * mat[13] -
                mat[12] * mat[1] * mat[11] + mat[12] * mat[3] * mat[9];
            inv_m[13] = mat[0] * mat[9] * mat[14] - mat[0] * mat[10] * mat[13] - mat[8] * mat[1] * mat[14] + mat[8] * mat[2] * mat[13] +
                mat[12] * mat[1] * mat[10] - mat[12] * mat[2] * mat[9];
            inv_m[2] = mat[1] * mat[6] * mat[15] - mat[1] * mat[7] * mat[14] - mat[5] * mat[2] * mat[15] + mat[5] * mat[3] * mat[14] +
                mat[13] * mat[2] * mat[7] - mat[13] * mat[3] * mat[6];
            inv_m[6] = -mat[0] * mat[6] * mat[15] + mat[0] * mat[7] * mat[14] + mat[4] * mat[2] * mat[15] - mat[4] * mat[3] * mat[14] -
                mat[12] * mat[2] * mat[7] + mat[12] * mat[3] * mat[6];
            inv_m[10] = mat[0] * mat[5] * mat[15] - mat[0] * mat[7] * mat[13] - mat[4] * mat[1] * mat[15] + mat[4] * mat[3] * mat[13] +
                mat[12] * mat[1] * mat[7] - mat[12] * mat[3] * mat[5];
            inv_m[14] = -mat[0] * mat[5] * mat[14] + mat[0] * mat[6] * mat[13] + mat[4] * mat[1] * mat[14] - mat[4] * mat[2] * mat[13] -
                mat[12] * mat[1] * mat[6] + mat[12] * mat[2] * mat[5];
            inv_m[3] = -mat[1] * mat[6] * mat[11] + mat[1] * mat[7] * mat[10] + mat[5] * mat[2] * mat[11] - mat[5] * mat[3] * mat[10] -
                mat[9] * mat[2] * mat[7] + mat[9] * mat[3] * mat[6];
            inv_m[7] = mat[0] * mat[6] * mat[11] - mat[0] * mat[7] * mat[10] - mat[4] * mat[2] * mat[11] + mat[4] * mat[3] * mat[10] +
                mat[8] * mat[2] * mat[7] - mat[8] * mat[3] * mat[6];
            inv_m[11] = -mat[0] * mat[5] * mat[11] + mat[0] * mat[7] * mat[9] + mat[4] * mat[1] * mat[11] - mat[4] * mat[3] * mat[9] -
                mat[8] * mat[1] * mat[7] + mat[8] * mat[3] * mat[5];
            inv_m[15] = mat[0] * mat[5] * mat[10] - mat[0] * mat[6] * mat[9] - mat[4] * mat[1] * mat[10] + mat[4] * mat[2] * mat[9] +
                mat[8] * mat[1] * mat[6] - mat[8] * mat[2] * mat[5];

            f32 det = mat[0] * inv_m[0] + mat[1] * inv_m[4] + mat[2] * inv_m[8] + mat[3] * inv_m[12];

            if (det == 0.0f)
                return mat4(); // Return identity on singular matrix

            det = 1.0f / det;
            for (int i = 0; i < 16; i++)
                inv_m[i] *= det;

            return inv;
        }

        void invert() { *this = inversed(); }

        // Static factory functions
        static mat4 identity() { return mat4(); }

        static mat4 translation(const vec3& v) {
            mat4 result;
            result.m[12] = v.x;
            result.m[13] = v.y;
            result.m[14] = v.z;
            return result;
        }

        static mat4 scale(const vec3& v) {
            mat4 result;
            result.m[0] = v.x;
            result.m[5] = v.y;
            result.m[10] = v.z;
            return result;
        }

        static mat4 rotation_x(f32 angle) {
            mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[5] = c;
            result.m[6] = s;
            result.m[9] = -s;
            result.m[10] = c;
            return result;
        }

        static mat4 rotation_y(f32 angle) {
            mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[0] = c;
            result.m[2] = -s;
            result.m[8] = s;
            result.m[10] = c;
            return result;
        }

        static mat4 rotation_z(f32 angle) {
            mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[0] = c;
            result.m[1] = s;
            result.m[4] = -s;
            result.m[5] = c;
            return result;
        }

        static mat4 look_at(const vec3& eye, const vec3& target, const vec3& up) {
            vec3 z = (eye - target).normalized();
            vec3 x = vec3::cross(up, z).normalized();
            vec3 y = vec3::cross(z, x);

            mat4 result;
            result.m[0] = x.x;
            result.m[4] = x.y;
            result.m[8] = x.z;
            result.m[12] = -vec3::dot(x, eye);
            result.m[1] = y.x;
            result.m[5] = y.y;
            result.m[9] = y.z;
            result.m[13] = -vec3::dot(y, eye);
            result.m[2] = z.x;
            result.m[6] = z.y;
            result.m[10] = z.z;
            result.m[14] = -vec3::dot(z, eye);
            result.m[3] = 0;
            result.m[7] = 0;
            result.m[11] = 0;
            result.m[15] = 1;
            return result;
        }

        static mat4 perspective(f32 fov_y, f32 aspect, f32 near_z, f32 far_z) {
            f32 tan_half_fov = std::tan(fov_y * 0.5f);
            mat4 result;
            std::memset(result.m, 0, sizeof(result.m));

            result.m[0] = 1.0f / (aspect * tan_half_fov);
            result.m[5] = 1.0f / tan_half_fov;
            result.m[10] = far_z / (far_z - near_z);
            result.m[11] = 1.0f;
            result.m[14] = -(far_z * near_z) / (far_z - near_z);
            return result;
        }

        static mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z) {
            mat4 result;
            std::memset(result.m, 0, sizeof(result.m));

            result.m[0] = 2.0f / (right - left);
            result.m[5] = 2.0f / (top - bottom);
            result.m[10] = 1.0f / (far_z - near_z);
            result.m[12] = -(right + left) / (right - left);
            result.m[13] = -(top + bottom) / (top - bottom);
            result.m[14] = -near_z / (far_z - near_z);
            result.m[15] = 1.0f;
            return result;
        }

        // Quaternion-based transformations (implemented in mat4.cpp)
        static mat4 from_quaternion(const quat& q);
        static mat4 affine_transformation(const vec3& scale, const vec3& rotation_origin, const quat& rotation, const vec3& translation);
        static mat4 affine_transformation(const vec4& scale, const vec4& rotation_origin, const quat& rotation, const vec4& translation);

        // Determinant calculation (implemented in mat4.cpp)
        f32 determinant() const;
    };

} // namespace sf::math
