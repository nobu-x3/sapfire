#pragma once

#include "components/component.h"
#include "math/math.h"

namespace sf::components {
    class MovementComponent {
        RTTI;
        ENGINE_COMPONENT(MovementComponent)
    public:
        MovementComponent();
        MovementComponent(const MovementComponent&);
        MovementComponent(MovementComponent&&) noexcept;
        MovementComponent& operator=(const MovementComponent&);
        MovementComponent& operator=(MovementComponent&&) noexcept;

        inline sf::math::vec3 acceleration() const { return m_Acceleration; }
        inline void acceleration(const sf::math::vec3& acceleration) { m_Acceleration = acceleration; }
        inline sf::math::vec3 velocity() const { return m_Velocity; }
        inline void velocity(const sf::math::vec3& velocity) { m_Velocity = velocity; }

    private:
        void register_rtti();

    private:
        sf::math::vec3 m_Acceleration{0.0f, 0.0f, 0.0f};
        sf::math::vec3 m_Velocity{0.0f, 0.0f, 0.0f};
    };
} // namespace sf::components
