#include "engpch.h"

#include "assets/asset_manager.h"
#include "core/string_utils.h"
#include "nlohmann/json.hpp"

namespace sf::assets {

	AssetManager::AssetManager(const AssetManagerCreationDesc& desc) :
		m_Device(desc.device), m_MeshRegistry(desc.mesh_registry_path), m_TextureRegistry(desc.texture_registry_path) {}

	void AssetManager::load_runtime_texture(const stl::string& texture_path) {
		auto relative_path = fs::relative_path(texture_path);
		auto* texture = m_TextureRegistry.get(relative_path);
		if (!texture) {
			m_TextureRegistry.import_texture(m_Device, relative_path);
			texture = m_TextureRegistry.get(relative_path);
		}
		if (texture->data.dsv_index == sf::render::INVALID_DESCRIPTOR_INDEX || texture->data.srv_index == sf::render::INVALID_DESCRIPTOR_INDEX) {
			texture->data = m_Device->create_texture(sf::render::TextureCreationDesc{
				.usage = sf::render::TextureUsage::ShaderResource,
				.name = sf::string_utils::to_wstring(relative_path),
				.path = sf::string_utils::to_wstring(fs::full_path(texture_path)),
			});
		}
		if (texture) {
			const assets::TextureResource text_res{
				.gpu_idx = texture->data.srv_index,
			};
			m_TextureManager.add(relative_path, texture->uuid, text_res);
		}
	}

	void AssetManager::import_texture(const stl::string& path) {
		auto relative_path = fs::relative_path(path);
		auto* texture = m_TextureRegistry.get(relative_path);
		if (!texture) {
			m_TextureRegistry.import_texture(m_Device, relative_path);
			texture = m_TextureRegistry.get(relative_path);
		}
		if (texture->data.dsv_index == sf::render::INVALID_DESCRIPTOR_INDEX || texture->data.srv_index == sf::render::INVALID_DESCRIPTOR_INDEX) {
			texture->data = m_Device->create_texture(sf::render::TextureCreationDesc{
				.usage = sf::render::TextureUsage::ShaderResource,
				.name = sf::string_utils::to_wstring(relative_path),
				.path = sf::string_utils::to_wstring(fs::full_path(path)),
			});
		}
		if (texture) {
			const assets::TextureResource text_res{
				.gpu_idx = texture->data.srv_index,
			};
			m_TextureManager.add(relative_path, texture->uuid, text_res);
		}
	}

	bool AssetManager::is_texture_loaded_for_runtime(UUID uuid) {
		if (uuid == TextureRegistry::default_texture(m_Device)->uuid) {
			return true;
		}
		bool loaded = true;
		if (!m_TextureManager.uuid_to_path_map.contains(uuid)) {
			loaded = false;
		}
		stl::string& path = m_TextureManager.uuid_to_path_map[uuid];
		if (!m_TextureManager.texture_resources.contains(path)) {
			loaded = false;
		}
		return loaded;
	}

	void AssetManager::load_all_runtime_textures() {
		for (auto&& [path, asset] : m_TextureRegistry.path_asset_map()) {
			load_runtime_texture(path);
		}
	}

	stl::string AssetManager::to_string() {
		nlohmann::json j;
		j["texture_registry"] = nlohmann::json::parse(m_TextureRegistry.to_string());
		j["mesh_registry"] = nlohmann::json::parse(m_MeshRegistry.to_string());
		j["material_registry"] = nlohmann::json::parse(m_MaterialRegistry.to_string());
		return stl::string(mem::MemTag::Strings, j.dump());
	}

	void AssetManager::deserialize(const stl::string& data) {
		m_MeshRegistry.deserialize(data);
		m_TextureRegistry.deserialize(m_Device, data);
		m_MaterialRegistry.deserialize(m_Device, data);
	}

	void AssetManager::serialize(const MaterialAsset& asset) { m_MaterialRegistry.serialize(asset); }

	void sf::assets::AssetManager::serialize() {
		m_MeshRegistry.serialize();
		m_TextureRegistry.serialize();
		m_MaterialRegistry.serialize();
		load_all_runtime_textures();
	}
} // namespace sf::assets
