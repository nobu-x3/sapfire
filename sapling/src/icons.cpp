#include "icons.h"

namespace icons {

	sf::stl::unordered_map<sf::stl::string, sf::render::Texture> g_IconTextures{};
	sf::stl::unordered_map<sf::stl::string, ImTextureID> g_IconIDs{};

	const sf::render::Texture& get(const sf::stl::string& id) { return g_IconTextures[id]; }

	void add(sf::render::IGraphicsDevice* device, const sf::stl::wstring& path, const sf::stl::string& id) {
		if (!g_IconTextures.contains(id)) {
			g_IconTextures[id] = device->create_texture({
				.usage = sf::render::TextureUsage::ShaderResource,
                .name = path,
				.path = path,
			});
			g_IconIDs[id] = (ImTextureID)device->cbv_srv_uav_descriptor_heap()
								->descriptor_handle_from_index(g_IconTextures[id].srv_index)
								.gpu_descriptor_handle.ptr;
		}
	}

	ImTextureID get_im_id(const sf::stl::string& id) { return g_IconIDs[id]; }
} // namespace icons
