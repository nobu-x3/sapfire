#pragma once

#include "core/core.h"
#include "vec3.h"

namespace sf::math {

    // Axis-Aligned Bounding Box
    struct aabb {
        vec3 center{0.0f, 0.0f, 0.0f};
        vec3 extents{0.0f, 0.0f, 0.0f};

        aabb() = default;
        aabb(const vec3& center, const vec3& extents) : center(center), extents(extents) {}

        static aabb create_from_points(const vec3* points, size_t count) {
            if (count == 0) {
                return aabb();
            }

            vec3 min_point = points[0];
            vec3 max_point = points[0];

            for (size_t i = 1; i < count; ++i) {
                min_point.x = min_point.x < points[i].x ? min_point.x : points[i].x;
                min_point.y = min_point.y < points[i].y ? min_point.y : points[i].y;
                min_point.z = min_point.z < points[i].z ? min_point.z : points[i].z;

                max_point.x = max_point.x > points[i].x ? max_point.x : points[i].x;
                max_point.y = max_point.y > points[i].y ? max_point.y : points[i].y;
                max_point.z = max_point.z > points[i].z ? max_point.z : points[i].z;
            }

            vec3 center = (min_point + max_point) * 0.5f;
            vec3 extents = (max_point - min_point) * 0.5f;

            return aabb(center, extents);
        }

        // Get min corner
        vec3 min() const { return center - extents; }

        // Get max corner
        vec3 max() const { return center + extents; }
    };

} // namespace sf::math
