#pragma once
#include "skinned_data.h"

namespace sf {
	class ECManager;
	class Entity;
	namespace render {
		class IGraphicsDevice;
	}
} // namespace sf

namespace sf::anim {

	struct SFAPI AnimationResource {
		u32 cpu_idx{0};
		u32 gpu_idx{0};
	};

	using FinalTransformCache =
		stl::unordered_map<UUID, stl::unordered_map<UUID, stl::unordered_map<f32, stl::vector<sf::math::mat4>>>>;

	class SFAPI AnimationManager {
	public:
		explicit AnimationManager(sf::ECManager* ec, sf::render::IGraphicsDevice* device);
		void update(f32 delta_time);
		const stl::vector<sf::math::mat4>& final_transforms_for_current_clip(Entity entity) const;

	private:
		// UUID of Skinned Data is the key
		stl::unordered_map<UUID, SkinnedData> m_UuidSkinnedDataMap{};
		stl::unordered_map<UUID, AnimationResource> m_SkinnedDataResourceMap{};
		// Entity UUID is the key
		stl::unordered_map<UUID, stl::vector<sf::math::mat4>> m_EntityTransformsMap{};
		FinalTransformCache m_FinalTransformsCache{};
		sf::ECManager& m_ECManager;
		sf::render::IGraphicsDevice* m_Device;
	};
} // namespace sf::anim
