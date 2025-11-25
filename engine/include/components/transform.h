#pragma once
#include "components/component.h"
#include "core/rtti.h"
#include "math/math.h"

namespace sf::components {

    class SFAPI Transform {
        RTTI;
        ENGINE_COMPONENT(Transform);

    public:
        inline Transform() { register_rtti(); }
        inline Transform(i32 parent_index) : m_ParentIndex(parent_index) { register_rtti(); }
        void register_rtti();
        Transform(const Transform&);
        Transform(Transform&&) noexcept;
        Transform& operator=(const Transform&);
        Transform& operator=(Transform&&) noexcept;
        void update(stl::vector<Transform>& transforms);
        inline sf::math::mat4 transform() const { return m_Transform; }
        inline sf::math::vec4 position() const { return m_Position; }
        inline sf::math::quat rotation() const { return m_Rotation; }
        inline sf::math::vec3 euler_rotation() const { return m_EulerAngles; }
        inline sf::math::vec4 scale() const { return m_Scale; }
        inline sf::math::vec3 forward() const { return m_Forward; }
        inline sf::math::vec3 up() const { return m_Up; }
        inline sf::math::vec3 right() const { return m_Right; }
        inline sf::math::mat4 rotation_matrix() const { return m_RotationMatrix; }
        inline i32 parent() const { return m_ParentIndex; }
        inline void parent(i32 parent_index) {
            m_ParentIndex = parent_index;
            m_Dirty = true;
        }
        Transform& position(const sf::math::vec4& position);
        Transform& rotation(const sf::math::quat& rotation);
        Transform& euler_rotation(const sf::math::vec3& euler_rotation);
        Transform& scale(const sf::math::vec4& scale);

    private:
        sf::math::mat4 m_Transform{sf::math::mat4::identity()};
        sf::math::quat m_Rotation{sf::math::quat::identity()};
        sf::math::vec4 m_Position{0.0f, 0.0f, 0.0f, 0.0f};
        sf::math::vec4 m_Scale{1.f, 1.f, 1.f, 1.f};
        sf::math::vec3 m_Right{1.0f, 0.0f, 0.0f};
        sf::math::vec3 m_Forward{0.0f, 0.0f, 1.0f};
        sf::math::vec3 m_Up{0.0f, 1.0f, 0.0f};
        sf::math::vec3 m_EulerAngles{0.0f, 0.0f, 0.0f};
        sf::math::mat4 m_RotationMatrix{sf::math::mat4::identity()};
        i32 m_ParentIndex{-1};
        bool m_Dirty{true};
    };
} // namespace sf::components
