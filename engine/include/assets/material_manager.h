#pragma once

#include "core/uuid.h"
#include "render/material.h"

namespace sf {
    namespace render {
        class IGraphicsDevice;
        class IMemoryAllocator;
        class IDescriptorHeap;
    }
} // namespace sf

namespace sf::assets {

    struct SFAPI MaterialAsset {
        UUID uuid;
        sf::render::Material material;
    };

    struct SFAPI MaterialResource {
        u32 cpu_idx{0};
        u32 gpu_idx{0};
    };

    struct SFAPI MaterialManager {
        sf::stl::unordered_map<sf::stl::string, MaterialResource> material_resources;
        sf::stl::unordered_map<sf::UUID, sf::stl::string> uuid_to_path_map;
        void add(const sf::stl::string& path, sf::UUID uuid, MaterialResource resource);
    };

    class SFAPI MaterialRegistry {
    public:
        explicit MaterialRegistry(const stl::string& registry_file_path);
        explicit MaterialRegistry() = default;
        ~MaterialRegistry() = default;
        MaterialRegistry(const MaterialRegistry&) = delete;
        MaterialRegistry(MaterialRegistry&&) = delete;
        MaterialRegistry& operator=(const MaterialRegistry&) = delete;
        MaterialRegistry& operator=(MaterialRegistry&&) = delete;
        stl::result<> import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap, const stl::string& path);
        stl::result<> import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap, MaterialAsset&& asset, const stl::string& path);
        stl::result<> import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap, const stl::string& path, UUID uuid);
        stl::result<> move_material(sf::render::IGraphicsDevice* device, const stl::string& old_path, const stl::string& new_path);
        stl::result<> release_material(const stl::string& path);
        void serialize();
        void serialize(const MaterialAsset& asset) const;
        void deserialize(sf::render::IGraphicsDevice* device, const stl::string& data);
        MaterialAsset* get(const stl::string& path) const;
        MaterialAsset* get(UUID uuid) const;
        stl::string get_path(UUID uuid) const;
        stl::unordered_map<stl::string, MaterialAsset>& path_asset_map() { return m_PathToMaterialAssetMap; }
        stl::string to_string();

        static MaterialAsset* default_material(sf::render::IGraphicsDevice* device = nullptr);

        static void create_default(const stl::string& registry_file_path);

    private:
        stl::string m_RegistryFilePath;
        stl::unordered_map<stl::string, MaterialAsset> m_PathToMaterialAssetMap{};
        stl::unordered_map<UUID, stl::string> m_UUIDToPathMap{};
    };
} // namespace sf::assets
