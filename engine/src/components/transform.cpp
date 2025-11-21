#include "engpch.h"

#include "components/transform.h"
#include "math/math.h"
#include "core/rtti.h"
#include "tools/profiling.h"

namespace Sapfire::components {

	ENGINE_COMPONENT_IMPL(Transform);

	Transform::Transform(const Transform& other) {
		m_Transform = other.m_Transform;
		m_Rotation = other.m_Rotation;
		m_Position = other.m_Position;
		m_Scale = other.m_Scale;
		m_Right = other.m_Right;
		m_Forward = other.m_Forward;
		m_Up = other.m_Up;
		m_EulerAngles = other.m_EulerAngles;
		m_RotationMatrix = other.m_RotationMatrix;
		m_ParentIndex = other.m_ParentIndex;
		m_Dirty = other.m_Dirty;
		register_rtti();
	}

	Transform::Transform(Transform&& other) noexcept {
		m_Transform = std::move(other.m_Transform);
		m_Rotation = std::move(other.m_Rotation);
		m_Position = std::move(other.m_Position);
		m_Scale = std::move(other.m_Scale);
		m_Right = std::move(other.m_Right);
		m_Forward = std::move(other.m_Forward);
		m_Up = std::move(other.m_Up);
		m_EulerAngles = std::move(other.m_EulerAngles);
		m_RotationMatrix = std::move(other.m_RotationMatrix);
		m_ParentIndex = std::move(other.m_ParentIndex);
		m_Dirty = std::move(other.m_Dirty);
		register_rtti();
	}

	Transform& Transform::operator=(const Transform& other) {
		m_Transform = other.m_Transform;
		m_Rotation = other.m_Rotation;
		m_Position = other.m_Position;
		m_Scale = other.m_Scale;
		m_Right = other.m_Right;
		m_Forward = other.m_Forward;
		m_Up = other.m_Up;
		m_EulerAngles = other.m_EulerAngles;
		m_RotationMatrix = other.m_RotationMatrix;
		m_ParentIndex = other.m_ParentIndex;
		m_Dirty = other.m_Dirty;
		register_rtti();
		return *this;
	}

	Transform& Transform::operator=(Transform&& other) noexcept {
		m_Transform = std::move(other.m_Transform);
		m_Rotation = std::move(other.m_Rotation);
		m_Position = std::move(other.m_Position);
		m_Scale = std::move(other.m_Scale);
		m_Right = std::move(other.m_Right);
		m_Forward = std::move(other.m_Forward);
		m_Up = std::move(other.m_Up);
		m_EulerAngles = std::move(other.m_EulerAngles);
		m_RotationMatrix = std::move(other.m_RotationMatrix);
		m_ParentIndex = std::move(other.m_ParentIndex);
		m_Dirty = std::move(other.m_Dirty);
		register_rtti();
		return *this;
	}

	void Transform::register_rtti() {
		BEGIN_RTTI()
		ADD_RTTI_FIELD(rtti ::rtti_type ::VEC3, "Position", &m_Position, [this]() { m_Dirty = true; })
		ADD_RTTI_FIELD(rtti ::rtti_type ::VEC3, "Scale", &m_Scale, [this]() { m_Dirty = true; })
		ADD_RTTI_FIELD(rtti ::rtti_type ::VEC3, "Euler Rotation", &m_EulerAngles, [this]() { this->euler_rotation(m_EulerAngles); })
		END_RTTI()
	}

	void Transform::update(stl::vector<Transform>& transforms) {
		PROFILE_FUNCTION();
		m_Transform = sf::math::mat4::affine_transformation(
			sf::math::vec3(m_Scale.x, m_Scale.y, m_Scale.z),
			sf::math::vec3(m_Position.x, m_Position.y, m_Position.z),
			m_Rotation,
			sf::math::vec3(m_Position.x, m_Position.y, m_Position.z)
		);
		if (m_ParentIndex >= 0) {
			m_Transform = transforms[m_ParentIndex].m_Transform * m_Transform;
		}
	}

	Transform& Transform::position(const sf::math::vec4& position) {
		PROFILE_FUNCTION();
		m_Position = position;
		m_Dirty = true;
		return *this;
	}

	Transform& Transform::rotation(const sf::math::quat& rotation) {
		PROFILE_FUNCTION();
		m_Rotation = rotation;
		m_RotationMatrix = sf::math::mat4::from_quaternion(rotation);
		m_Forward = m_Rotation.rotate(sf::math::vec3{0, 0, 1});
		m_Right = m_Rotation.rotate(sf::math::vec3{1, 0, 0});
		m_Up = sf::math::vec3::cross(m_Right, m_Forward).normalized();
		m_Dirty = true;
		return *this;
	}

	Transform& Transform::euler_rotation(const sf::math::vec3& euler_rotation) {
		PROFILE_FUNCTION();
		m_EulerAngles = euler_rotation;
		m_Rotation = sf::math::quat::from_euler(m_EulerAngles);
		m_RotationMatrix = sf::math::mat4::from_quaternion(m_Rotation);
		m_Forward = sf::math::vec3{0, 0, 1};
		m_Forward = m_RotationMatrix.transform_vector(m_Forward);
		m_Right = sf::math::vec3{1, 0, 0};
		m_Right = m_RotationMatrix.transform_vector(m_Right);
		m_Up = sf::math::vec3::cross(m_Right, m_Forward).normalized();
		m_Dirty = true;
		return *this;
	}

	Transform& Transform::scale(const sf::math::vec4& scale) {
		PROFILE_FUNCTION();
		m_Scale = scale;
		return *this;
	}

} // namespace Sapfire::components
