#pragma once
#include "components/movement_component.h"
#include "components/transform.h"
#include "core/input.h"
#include "math/math.h"

namespace sf {
    struct SFAPI Camera {
        Camera() = default;
        Camera(f32 fov, f32 aspect, f32 near_plane, f32 far_plane);
        void update(f32);
        components::Transform transform{};
        sf::math::mat4 projection{sf::math::mat4::identity()};
        sf::math::mat4 view() const;
        components::MovementComponent movement_component;
        input::InputComponent input;
        f32 fov{};
        f32 aspect{};
        f32 near_plane{};
        f32 far_plane{};
    };
} // namespace sf
