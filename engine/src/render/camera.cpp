#include "engpch.h"

#include "render/camera.h"
#include "math/math.h"

namespace sf {

	Camera::Camera(f32 fov, f32 aspect, f32 near_plane, f32 far_plane) :
		fov(fov), aspect(aspect), near_plane(near_plane), far_plane(far_plane) {
		projection = sf::math::mat4::perspective(fov, aspect, near_plane, far_plane);
	}

	sf::math::mat4 Camera::view() const {
		PROFILE_FUNCTION();
		auto translation_rotation =
			sf::math::mat4::identity() * transform.rotation_matrix() * sf::math::mat4::translation(sf::math::vec3(transform.position().x, transform.position().y, transform.position().z));
		return translation_rotation.inversed();
	}

	void Camera::update(f32 delta_time) {
		PROFILE_FUNCTION();
		if (input.mouse_state.RMB) {
			const sf::math::vec3 mouse_delta{-input.mouse_delta_y, -input.mouse_delta_x, 0.f};
			auto euler = transform.euler_rotation();
			euler = euler + mouse_delta;
			transform.euler_rotation(euler);
		}
		auto velocity = movement_component.velocity();
		velocity = transform.forward() * input.input_axis.x + transform.right() * input.input_axis.y + transform.up() * input.input_axis.z;
		movement_component.velocity(velocity);
		auto position = transform.position();
		position = position + sf::math::vec4(velocity * delta_time, 0.0f);
		transform.position(position);
		stl::vector<components::Transform> t{};
		transform.update(t);
	}
} // namespace sf
