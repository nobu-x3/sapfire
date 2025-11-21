#pragma once

#include "vec3.h"
#include "vec4.h"
#include "mat4.h"
#include "aabb.h"
#include "core/core.h"

namespace sf::math {

// Frustum intersection result
enum class ContainmentType {
    Disjoint = 0,  // No intersection
    Intersects,    // Partial intersection
    Contains       // Fully contained
};

// Frustum represented by 6 planes (left, right, top, bottom, near, far)
// Each plane is stored as vec4(a, b, c, d) where ax + by + cz + d = 0
struct frustum {
    vec4 planes[6];

    frustum() = default;

    // Create frustum from projection matrix
    static frustum create_from_matrix(const mat4& projection) {
        frustum f;

        // Left plane
        f.planes[0] = vec4(
            projection.m2[0][3] + projection.m2[0][0],
            projection.m2[1][3] + projection.m2[1][0],
            projection.m2[2][3] + projection.m2[2][0],
            projection.m2[3][3] + projection.m2[3][0]
        );

        // Right plane
        f.planes[1] = vec4(
            projection.m2[0][3] - projection.m2[0][0],
            projection.m2[1][3] - projection.m2[1][0],
            projection.m2[2][3] - projection.m2[2][0],
            projection.m2[3][3] - projection.m2[3][0]
        );

        // Bottom plane
        f.planes[2] = vec4(
            projection.m2[0][3] + projection.m2[0][1],
            projection.m2[1][3] + projection.m2[1][1],
            projection.m2[2][3] + projection.m2[2][1],
            projection.m2[3][3] + projection.m2[3][1]
        );

        // Top plane
        f.planes[3] = vec4(
            projection.m2[0][3] - projection.m2[0][1],
            projection.m2[1][3] - projection.m2[1][1],
            projection.m2[2][3] - projection.m2[2][1],
            projection.m2[3][3] - projection.m2[3][1]
        );

        // Near plane
        f.planes[4] = vec4(
            projection.m2[0][2],
            projection.m2[1][2],
            projection.m2[2][2],
            projection.m2[3][2]
        );

        // Far plane
        f.planes[5] = vec4(
            projection.m2[0][3] - projection.m2[0][2],
            projection.m2[1][3] - projection.m2[1][2],
            projection.m2[2][3] - projection.m2[2][2],
            projection.m2[3][3] - projection.m2[3][2]
        );

        // Normalize planes
        for (int i = 0; i < 6; i++) {
            f32 length = std::sqrt(
                f.planes[i].x * f.planes[i].x +
                f.planes[i].y * f.planes[i].y +
                f.planes[i].z * f.planes[i].z
            );
            if (length > 0.0f) {
                f.planes[i] = f.planes[i] * (1.0f / length);
            }
        }

        return f;
    }

    // Transform frustum by a matrix
    frustum transform(const mat4& matrix) const {
        // Transform by inverse transpose for plane equations
        mat4 inv_transpose = matrix.inversed().transposed();

        frustum result;
        for (int i = 0; i < 6; i++) {
            result.planes[i] = inv_transpose * planes[i];

            // Re-normalize
            f32 length = std::sqrt(
                result.planes[i].x * result.planes[i].x +
                result.planes[i].y * result.planes[i].y +
                result.planes[i].z * result.planes[i].z
            );
            if (length > 0.0f) {
                result.planes[i] = result.planes[i] * (1.0f / length);
            }
        }
        return result;
    }

    // Test if AABB is contained in frustum
    ContainmentType contains(const aabb& box) const {
        vec3 min_point = box.min();
        vec3 max_point = box.max();

        // Get box corners
        vec3 corners[8] = {
            vec3(min_point.x, min_point.y, min_point.z),
            vec3(max_point.x, min_point.y, min_point.z),
            vec3(min_point.x, max_point.y, min_point.z),
            vec3(max_point.x, max_point.y, min_point.z),
            vec3(min_point.x, min_point.y, max_point.z),
            vec3(max_point.x, min_point.y, max_point.z),
            vec3(min_point.x, max_point.y, max_point.z),
            vec3(max_point.x, max_point.y, max_point.z)
        };

        bool intersecting = false;

        // Test each plane
        for (int p = 0; p < 6; p++) {
            int in_count = 8;
            int point_in = 1;

            // Test all corners against this plane
            for (int i = 0; i < 8; i++) {
                // Point-to-plane distance test
                f32 distance =
                    planes[p].x * corners[i].x +
                    planes[p].y * corners[i].y +
                    planes[p].z * corners[i].z +
                    planes[p].w;

                if (distance < 0.0f) {
                    point_in = 0;
                    in_count--;
                }
            }

            // If all points are outside one plane, the box is outside
            if (in_count == 0) {
                return ContainmentType::Disjoint;
            }

            // If some points are outside, we're intersecting
            if (point_in == 0) {
                intersecting = true;
            }
        }

        // If we're intersecting, return intersects, otherwise fully contained
        return intersecting ? ContainmentType::Intersects : ContainmentType::Contains;
    }

    // Test if point is contained in frustum
    bool contains(const vec3& point) const {
        for (int i = 0; i < 6; i++) {
            f32 distance =
                planes[i].x * point.x +
                planes[i].y * point.y +
                planes[i].z * point.z +
                planes[i].w;

            if (distance < 0.0f) {
                return false;
            }
        }
        return true;
    }
};

} // namespace sf::math
