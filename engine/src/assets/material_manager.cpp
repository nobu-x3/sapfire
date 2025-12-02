#include "engpch.h"

#include "assets/material_manager.h"
#include "core/file_system.h"
#include "core/logger.h"
#include "core/string_utils.h"
#include "math/math.h"
#include "nlohmann/json.hpp"
#include "render/i_graphics_device.h"

namespace sf::assets {
    const UUID DEFAULT_MATERIAL_UUID = 13700118063961433559ULL;
    constexpr f32 DEFAULT_MATERIAL_ROUGHTNESS = 0.f;
    constexpr sf::math::vec4 DEFAULT_MATERIAL_ALBEDO = {1.f, 0.f, 1.f, 1.f};
    constexpr sf::math::vec3 DEFAULT_MATERIAL_FRESNEL = {1.f, 1.f, 1.f};
    const char* DEFAULT_MATERIAL_NAME = "Default Material";

    void MaterialManager::add(const sf::stl::string& path, sf::UUID uuid, MaterialResource resource) {
        material_resources[path] = resource;
        uuid_to_path_map[uuid] = path;
    }

    MaterialRegistry::MaterialRegistry(const stl::string& registry_file_path) : m_RegistryFilePath(fs::full_path(registry_file_path)) {}

    void MaterialRegistry::create_default(const stl::string& filepath) {
        auto j = R"(
        {
            "assets" : []
        }
        )"_json;
        std::ofstream file{filepath.c_str()};
        file << std::setw(4) << j << std::endl;
        file.close();
    }

    stl::result<> MaterialRegistry::import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap,
                                                    const stl::string& path) {
        if (m_PathToMaterialAssetMap.contains(path))
            return stl::success;
        std::ifstream file{path.c_str()};
        if (!file.is_open()) {
            return stl::make_error("Material at path {} could not be open.", path.data());
        }
        nlohmann::json j;
        file >> j;
        file.close();
        if (!j.contains("UUID")) {
            return stl::make_error("Broken material at path {}. Does not contain UUID.", path.data());
        }
        if (!j.contains("name")) {
            return stl::make_error("Broken material at path {}. Does not contain name.", path.data());
        }
        if (!j.contains("diffuse_albedo")) {
            return stl::make_error("Broken material at path {}. Does not contain diffuse albedo.", path.data());
        }
        if (!j.contains("fresnel_r0")) {
            return stl::make_error("Broken material at path {}. Does not contain fresnel r0.", path.data());
        }
        if (!j.contains("roughness")) {
            return stl::make_error("Broken material at path {}. Does not contain roughness.", path.data());
        }
        const UUID uuid = UUID{j["UUID"]};
        stl::string name_str = j["name"];
        sf::render::Material material{.name = fs::file_name(name_str)};
        material.roughness = j["roughness"];
        material.diffuse_albedo =
            sf::math::vec4(j["diffuse_albedo"][0], j["diffuse_albedo"][1], j["diffuse_albedo"][2], j["diffuse_albedo"][3]);
        auto result = allocator->allocate_buffer({
            .usage = sf::render::BufferUsage::Constant,
            .size_in_bytes = sizeof(sf::render::MaterialConstants),
            .name = material.name,
        });
        if (!result) {
            return stl::make_error("Failed to create buffer for material at path {}: {}", path.data(), result.error().data());
        }
        material.fresnel_r0 = sf::math::vec3(j["fresnel_r0"][0], j["fresnel_r0"][1], j["fresnel_r0"][2]);
        result->cbv_index = heap->allocate_cbv(*result);
        material.material_buffer = std::move(*result);
        material.material_cb_index = material.material_buffer.cbv_index;
        m_PathToMaterialAssetMap[path] = MaterialAsset{
            .uuid = uuid,
            .material = material,
        };
        m_UUIDToPathMap[uuid] = path;
        return stl::success;
    }

    stl::result<> MaterialRegistry::import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap,
                                                    const stl::string& path, UUID uuid) {
        if (m_PathToMaterialAssetMap.contains(path))
            return stl::success;
        std::ifstream file{path.c_str()};
        if (!file.is_open()) {
            return stl::make_error("Material at path {} could not be open.", path.data());
        }
        nlohmann::json j;
        file >> j;
        file.close();
        if (!j.contains("name")) {
            return stl::make_error("Broken material at path {}. Does not contain name.", path.data());
        }
        if (!j.contains("diffuse_albedo")) {
            return stl::make_error("Broken material at path {}. Does not contain diffuse albedo.", path.data());
        }
        if (!j.contains("fresnel_r0")) {
            return stl::make_error("Broken material at path {}. Does not contain fresnel r0.", path.data());
        }
        if (!j.contains("roughness")) {
            return stl::make_error("Broken material at path {}. Does not contain roughness.", path.data());
        }
        stl::string name_str = j["name"];
        sf::render::Material material{.name = fs::file_name(name_str)};
        material.roughness = j["roughness"];
        material.diffuse_albedo =
            sf::math::vec4(j["diffuse_albedo"][0], j["diffuse_albedo"][1], j["diffuse_albedo"][2], j["diffuse_albedo"][3]);
        material.fresnel_r0 = sf::math::vec3(j["fresnel_r0"][0], j["fresnel_r0"][1], j["fresnel_r0"][2]);
        auto buffer_result = allocator->allocate_buffer({
            .usage = sf::render::BufferUsage::Constant,
            .size_in_bytes = sizeof(sf::render::MaterialConstants),
            .name = material.name,
        });
        if (!buffer_result) {
            return stl::make_error("Failed to create material buffer for material at path {}: {}", path.data(),
                                   buffer_result.error().data());
        }
        buffer_result->cbv_index = heap->allocate_cbv(*buffer_result);
        material.material_buffer = std::move(*buffer_result);
        material.material_cb_index = material.material_buffer.cbv_index;
        m_PathToMaterialAssetMap[path] = MaterialAsset{
            .uuid = uuid,
            .material = material,
        };
        m_UUIDToPathMap[uuid] = path;
        return stl::success;
    }

    stl::result<> MaterialRegistry::import_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap,
                                                    MaterialAsset&& asset, const stl::string& path) {
        if (m_PathToMaterialAssetMap.contains(path)) {
            return stl::success;
        }
        auto buffer_result = allocator->allocate_buffer({
            .usage = sf::render::BufferUsage::Constant,
            .size_in_bytes = sizeof(sf::render::MaterialConstants),
            .name = asset.material.name,
        });
        if (!buffer_result) {
            return stl::make_error("Failed to create material buffer for material at path {}: {}", path.data(),
                                   buffer_result.error().data());
        }
        buffer_result->cbv_index = heap->allocate_cbv(*buffer_result);
        asset.material.name = fs::file_name(path);
        asset.material.material_buffer = std::move(*buffer_result);
        asset.material.material_cb_index = asset.material.material_buffer.cbv_index;
        const UUID uuid{asset.uuid};
        m_PathToMaterialAssetMap[path] = std::move(asset);
        m_UUIDToPathMap[uuid] = path;
        return stl::success;
    }

    stl::result<> MaterialRegistry::move_material(sf::render::IGraphicsDevice* device, const stl::string& old_path,
                                                  const stl::string& new_path) {
        assert("Not implemented yet");
        return stl::success;
    }

    stl::result<> MaterialRegistry::release_material(const stl::string& path) {
        if (!m_PathToMaterialAssetMap.contains(path)) {
            return stl::make_error("Material at path {} does not exist.", path.data());
        }
        auto uuid = m_PathToMaterialAssetMap[path].uuid;
        m_PathToMaterialAssetMap.erase(path);
        m_UUIDToPathMap.erase(uuid);
        return stl::success;
    }

    void MaterialRegistry::serialize() {
        nlohmann::json j;
        for (auto&& [path, asset] : m_PathToMaterialAssetMap) {
            auto& uuid = asset.uuid;
            auto& material = asset.material;
            auto& name = material.name;
            auto& roughness = material.roughness;
            float fresnel_r0[3] = {material.fresnel_r0.x, material.fresnel_r0.y, material.fresnel_r0.z};
            float diffuse_albedo[4] = {material.diffuse_albedo.x, material.diffuse_albedo.y, material.diffuse_albedo.z,
                                       material.diffuse_albedo.w};
            const nlohmann::json j_obj = {
                {"UUID", static_cast<u64>(uuid)},   {"path", path}, {"name", name}, {"roughness", roughness}, {"fresnel_r0", fresnel_r0},
                {"diffuse_albedo", diffuse_albedo},
            };
            {
                // @TODO: this is super slow because opening files in a loop. Rework this.
                std::ofstream file{path.c_str()};
                file.clear();
                file << std::setw(4) << j_obj << std::endl;
                file.close();
            }
            {
                const nlohmann::json path_jobj = {"path", path};
                j["assets"].push_back(path_jobj);
            }
        }
        std::ofstream file{m_RegistryFilePath.c_str()};
        file.clear();
        file << std::setw(4) << j << std::endl;
        file.close();
    }

    void MaterialRegistry::serialize(const MaterialAsset& asset) const {
        auto& uuid = asset.uuid;
        if (!m_UUIDToPathMap.contains(uuid)) {
            CORE_ERROR("Material asset with UUID {} has not been previously imported to the registry.", static_cast<u64>(uuid));
            return;
        }
        const auto& path = m_UUIDToPathMap.at(uuid);
        auto& material = asset.material;
        auto& name = material.name;
        auto& roughness = material.roughness;
        float fresnel_r0[3] = {material.fresnel_r0.x, material.fresnel_r0.y, material.fresnel_r0.z};
        float diffuse_albedo[4] = {material.diffuse_albedo.x, material.diffuse_albedo.y, material.diffuse_albedo.z,
                                   material.diffuse_albedo.w};
        const nlohmann::json j_obj = {
            {"UUID", static_cast<u64>(uuid)},   {"path", path}, {"name", name}, {"roughness", roughness}, {"fresnel_r0", fresnel_r0},
            {"diffuse_albedo", diffuse_albedo},
        };
        {
            // @TODO: this is super slow because opening files in a loop. Rework this.
            std::ofstream file{fs::full_path(path).c_str()};
            file.clear();
            file << std::setw(4) << j_obj << std::endl;
            file.close();
        }
    }

    void MaterialRegistry::deserialize(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap,
                                       const stl::string& data) {
        nlohmann::json j = nlohmann::json::parse(data)["assets"];
        for (auto&& asset : j["material_registry"]) {
            if (!asset.contains("path")) {
                CORE_CRITICAL("Broken material registry. At least one registry entry does not contain path to the raw asset.");
                return;
            }
            stl::string path = asset["path"];
            stl::string relative_path = fs::relative_path(path);
            if (relative_path.empty()) {
                CORE_WARN("Could not locate material at path {}.", path);
                continue;
            }
            if (!fs::exists(relative_path)) {
                CORE_ERROR("Material with path {} does not exist.", relative_path);
                continue;
            }
            import_material(allocator, heap, relative_path);
        }
    }

    stl::string MaterialRegistry::to_string() {
        nlohmann::json j;
        for (auto&& [path, asset] : m_PathToMaterialAssetMap) {
            auto& uuid = asset.uuid;
            auto& material = asset.material;
            auto& name = material.name;
            auto& roughness = material.roughness;
            float fresnel_r0[3] = {material.fresnel_r0.x, material.fresnel_r0.y, material.fresnel_r0.z};
            float diffuse_albedo[4] = {material.diffuse_albedo.x, material.diffuse_albedo.y, material.diffuse_albedo.z,
                                       material.diffuse_albedo.w};
            const nlohmann::json j_obj = {
                {"UUID", static_cast<u64>(uuid)},   {"path", path}, {"name", name}, {"roughness", roughness}, {"fresnel_r0", fresnel_r0},
                {"diffuse_albedo", diffuse_albedo},
            };
            const nlohmann::json path_jobj = {{"path", path}};
            j.push_back(path_jobj);
        }
        return stl::string(mem::MemTag::Strings, j.dump());
    }

    MaterialAsset* MaterialRegistry::get(const stl::string& path) const {
        if (!m_PathToMaterialAssetMap.contains(path))
            return MaterialRegistry::default_material();
        return const_cast<MaterialAsset*>(&m_PathToMaterialAssetMap.at(path));
    }

    MaterialAsset* MaterialRegistry::get(UUID uuid) const {
        if (!m_UUIDToPathMap.contains(uuid)) {
            return MaterialRegistry::default_material();
        }
        auto& path = m_UUIDToPathMap.at(uuid);
        return get(path);
    }

    stl::string MaterialRegistry::get_path(UUID uuid) const {
        if (!m_UUIDToPathMap.contains(uuid))
            return "";
        return m_UUIDToPathMap.at(uuid);
    }

    MaterialAsset* MaterialRegistry::default_material(sf::render::IMemoryAllocator* allocator, sf::render::IDescriptorHeap* heap) {
        const static std::string name = DEFAULT_MATERIAL_NAME;
        static sf::render::MaterialConstants default_material_constants{
            .diffuse_albedo = DEFAULT_MATERIAL_ALBEDO,
            .fresnel_r0 = DEFAULT_MATERIAL_FRESNEL,
            .roughness = DEFAULT_MATERIAL_ROUGHTNESS,
        };
        static auto buffer_result = allocator->allocate_buffer({
            .usage = sf::render::BufferUsage::Constant,
            .size_in_bytes = sizeof(sf::render::MaterialConstants),
            .name = name,
        });
        if (!buffer_result) {
            CORE_CRITICAL("Failed to load default material buffer.");
            return nullptr;
        }
        buffer_result->cbv_index = heap->allocate_cbv(*buffer_result);
        static MaterialAsset default_mat{
            .uuid = DEFAULT_MATERIAL_UUID,
            .material{
                .name = DEFAULT_MATERIAL_NAME,
                .diffuse_albedo = DEFAULT_MATERIAL_ALBEDO,
                .fresnel_r0 = DEFAULT_MATERIAL_FRESNEL,
                .roughness = DEFAULT_MATERIAL_ROUGHTNESS,
                .material_buffer = *buffer_result,
                .material_cb_index = static_cast<i32>(buffer_result->cbv_index),
            },
        };
        buffer_result->update(&default_material_constants, sizeof(sf::render::MaterialConstants));
        return &default_mat;
    }
} // namespace sf::assets
