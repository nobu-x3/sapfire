#pragma once

#include "core/core.h"
#include "core/uuid.h"
#include "render/render_backend.h"

namespace sf {
    namespace render {
        class IGraphicsDevice;
    }
} // namespace sf

namespace sf::assets {

    struct SFAPI TextureResource {
        u32 cpu_idx{0};
        u32 gpu_idx{0};
    };

    struct SFAPI TextureManager {
        sf::stl::unordered_map<sf::stl::string, TextureResource> texture_resources;
        sf::stl::unordered_map<sf::UUID, sf::stl::string> uuid_to_path_map;
        void add(const sf::stl::string& path, sf::UUID uuid, TextureResource resource);
    };

    struct SFAPI TextureAsset {
        UUID uuid;
        sf::render::TextureCreationDesc description;
        sf::render::Texture data;
    };

    class SFAPI TextureRegistry {
    public:
        explicit TextureRegistry(const stl::string& texture_registry_path);
        explicit TextureRegistry() = default;
        ~TextureRegistry() = default;
        TextureRegistry(const TextureRegistry&) = delete;
        TextureRegistry(TextureRegistry&&) = delete;
        TextureRegistry& operator=(const TextureRegistry&) = delete;
        TextureRegistry& operator=(TextureRegistry&&) = delete;
        void import_texture(sf::render::IGraphicsDevice* device, const stl::string& path);
        void import_texture(sf::render::IGraphicsDevice* device, const stl::string& path, const sf::render::TextureCreationDesc& desc,
                            UUID uuid = {});
        void move_texture(sf::render::IGraphicsDevice* device, const stl::string& old_path, const stl::string& new_path);
        void release_texture(const stl::string& path);
        void serialize();
        void deserialize(sf::render::IGraphicsDevice* device, const stl::string& data);
        TextureAsset* get(const stl::string& path) const;
        TextureAsset* get(UUID uuid) const;
        stl::string get_path(UUID uuid) const;
        const stl::unordered_map<stl::string, TextureAsset>& path_asset_map() const { return m_PathToTextureAssetMap; }
        stl::string to_string();

        static void create_default(const stl::string& registry_file_path);
        static TextureAsset* default_texture(sf::render::IGraphicsDevice* device = nullptr);

    private:
        stl::string m_RegistryFilePath;
        stl::unordered_map<stl::string, TextureAsset> m_PathToTextureAssetMap{};
        stl::unordered_map<UUID, stl::string> m_UUIDToPathMap{};
    };
} // namespace sf::assets
