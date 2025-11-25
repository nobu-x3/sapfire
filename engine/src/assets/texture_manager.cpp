#include "core/file_system.h"
#include "engpch.h"

#include "assets/texture_manager.h"
#include "core/string_utils.h"
#include "nlohmann/json.hpp"
#include "tools/texture_loader.h"

namespace sf::assets {

    void TextureManager::add(const sf::stl::string& path, sf::UUID uuid, TextureResource resource) {
        texture_resources[path] = resource;
        uuid_to_path_map[uuid] = path;
    }
    TextureRegistry::TextureRegistry(const stl::string& registry_file_path) : m_RegistryFilePath(fs::full_path(registry_file_path)) {}

    void TextureRegistry::create_default(const stl::string& filepath) {
        auto j = R"(
        {
            "assets" : []
        }
        )"_json;
        std::ofstream file{filepath.c_str()};
        file << std::setw(4) << j << std::endl;
        file.close();
    }

    void TextureRegistry::import_texture(sf::render::IGraphicsDevice* device, const stl::string& path) {
        auto uuid = UUID{};
        i32 width{};
        i32 height{};
        void* data = tools::texture_loader::load(fs::full_path(path).c_str(), width, height, 4);
        auto name = sf::string_utils::to_wstring(path);
        if (m_PathToTextureAssetMap.contains(path))
            return;
        m_PathToTextureAssetMap[path] = TextureAsset{
            .uuid = uuid,
            .description =
                sf::render::TextureCreationDesc{
                    .usage = sf::render::TextureUsage::ShaderResource,
                    .width = static_cast<u32>(width),
                    .height = static_cast<u32>(height),
                    .name = name,
                    .path = sf::string_utils::to_wstring(path),
                },
            .data = device->create_texture_with_data(
                sf::render::TextureCreationDesc{
                    .usage = sf::render::TextureUsage::ShaderResource,
                    .width = static_cast<u32>(width),
                    .height = static_cast<u32>(height),
                    .name = name,
                    .path = sf::string_utils::to_wstring(fs::full_path(path)),
                },
                data, width * height * 4),
        };
        m_UUIDToPathMap[uuid] = path;
    }

    void TextureRegistry::import_texture(sf::render::IGraphicsDevice* device, const stl::string& path,
                                         const sf::render::TextureCreationDesc& desc, UUID uuid) {
        auto name = sf::string_utils::to_wstring(path);
        if (m_PathToTextureAssetMap.contains(path))
            return;
        m_PathToTextureAssetMap[path] = TextureAsset{
            .uuid = uuid,
            .description = desc,
            .data = device->create_texture(sf::render::TextureCreationDesc{
                .usage = sf::render::TextureUsage::ShaderResource,
                .name = name,
                .path = sf::string_utils::to_wstring(fs::full_path(path)),
            }),
        };
        m_UUIDToPathMap[uuid] = path;
    }

    void TextureRegistry::move_texture(sf::render::IGraphicsDevice* device, const stl::string& old_path, const stl::string& new_path) {
        if (!m_PathToTextureAssetMap.contains(old_path)) {
            CORE_WARN("Texture at path {} does not exist, adding new.", old_path);
            auto uuid = UUID{};
            i32 width{};
            i32 height{};
            void* data = tools::texture_loader::load(new_path.c_str(), width, height, 4);
            auto name = sf::string_utils::to_wstring(new_path);
            m_PathToTextureAssetMap[new_path] = TextureAsset{
                .uuid = uuid,
                .description =
                    sf::render::TextureCreationDesc{
                        .usage = sf::render::TextureUsage::ShaderResource,
                        .width = static_cast<u32>(width),
                        .height = static_cast<u32>(height),
                        .name = name,
                        .path = sf::string_utils::to_wstring(new_path),
                    },
                .data = device->create_texture_with_data(
                    sf::render::TextureCreationDesc{
                        .usage = sf::render::TextureUsage::ShaderResource,
                        .width = static_cast<u32>(width),
                        .height = static_cast<u32>(height),
                        .name = name,
                        .path = sf::string_utils::to_wstring(new_path),
                    },
                    data, width * height * 4),
            };
            m_UUIDToPathMap[uuid] = new_path;
            return;
        }
        auto mesh_data = m_PathToTextureAssetMap[old_path];
        m_PathToTextureAssetMap.erase(old_path);
        m_PathToTextureAssetMap[new_path] = mesh_data;
        m_UUIDToPathMap[mesh_data.uuid] = new_path;
    }

    void TextureRegistry::release_texture(const stl::string& path) {
        if (!m_PathToTextureAssetMap.contains(path)) {
            CORE_ERROR("Texture at path {} does not exist", path);
            return;
        }
        auto uuid = m_PathToTextureAssetMap[path].uuid;
        m_PathToTextureAssetMap.erase(path);
        m_UUIDToPathMap.erase(uuid);
    }

    void TextureRegistry::serialize() {
        nlohmann::json j;
        for (auto&& [path, asset] : m_PathToTextureAssetMap) {
            auto& desc = asset.description;
            nlohmann::json desc_j{
                {"usage", static_cast<u32>(desc.usage)},
                {"width", desc.width},
                {"height", desc.height},
                {"format", desc.format},
                {"mip_levels", desc.mip_levels},
                {"depth_or_array_size", desc.depth_or_array_size},
            };
            if (desc.initial_state != sf::render::ResourceState::Common) {
                desc_j["initial_state"] = static_cast<u32>(desc.initial_state);
            }
            const nlohmann::json j_obj = {{"UUID", static_cast<u64>(asset.uuid)}, {"path", path}, {"description", desc_j}};
            j["assets"].push_back(j_obj);
        }
        std::ofstream file{m_RegistryFilePath.c_str()};
        file.clear();
        file << std::setw(4) << j << std::endl;
        file.close();
    }
    TextureAsset* TextureRegistry::get(const stl::string& path) const {
        if (!m_PathToTextureAssetMap.contains(path))
            return TextureRegistry::default_texture();
        return const_cast<TextureAsset*>(&m_PathToTextureAssetMap.at(path));
    }
    TextureAsset* TextureRegistry::get(UUID uuid) const {
        if (!m_UUIDToPathMap.contains(uuid))
            return TextureRegistry::default_texture();
        auto& path = m_UUIDToPathMap.at(uuid);
        return get(path);
    }
    stl::string TextureRegistry::get_path(UUID uuid) const {
        if (!m_UUIDToPathMap.contains(uuid))
            return "";
        return m_UUIDToPathMap.at(uuid);
    }
    stl::string TextureRegistry::to_string() {
        nlohmann::json j;
        for (auto&& [path, asset] : m_PathToTextureAssetMap) {
            auto& desc = asset.description;
            nlohmann::json desc_j{
                {"usage", static_cast<u32>(desc.usage)},
                {"width", desc.width},
                {"height", desc.height},
                {"format", desc.format},
                {"mip_levels", desc.mip_levels},
                {"depth_or_array_size", desc.depth_or_array_size},
            };
            if (desc.initial_state != sf::render::ResourceState::Common) {
                desc_j["initial_state"] = static_cast<u32>(desc.initial_state);
            }
            const nlohmann::json j_obj = {{"UUID", static_cast<u64>(asset.uuid)}, {"path", path}, {"description", desc_j}};
            j.push_back(j_obj);
        }
        return stl::string(mem::MemTag::Strings, j.dump());
    }

    void TextureRegistry::deserialize(sf::render::IGraphicsDevice* device, const stl::string& data) {
        nlohmann::json j = nlohmann::json::parse(data)["assets"];
        if (!j.contains("texture_registry")) {
            CORE_CRITICAL("Given texture registry string for deserialization deos not contain texture registry. The texture registry will "
                          "not be loaded.");
            return;
        }
        for (auto&& asset : j["texture_registry"]) {
            if (!asset.contains("path")) {
                CORE_ERROR("One of the textures in the texture registry is missing a path to raw texture. It will not be loaded. Dump:\n{}",
                           data);
                continue;
            }
            stl::string path = asset["path"];
            if (!fs::exists(path)) {
                CORE_ERROR("Texture with path {} does not exist.", path);
                continue;
            }
            path = fs::relative_path(path);
            if (!asset.contains("UUID")) {
                CORE_ERROR("Texture with path {} is missing UUID at deserialization. It will not be loaded. Dump:\n{}", path, data);
                continue;
            }
            if (!asset.contains("description")) {
                CORE_ERROR("Texture with path {} is missing texture creation description. It will not be loaded. Dump:\n{}", path, data);
                continue;
            }
            const UUID uuid{asset["UUID"]};
            auto& description = asset["description"];
            if (!description.contains("usage")) {
                CORE_ERROR("Texture with path {} is missing texture creation description's 'usage' field. It will not be loaded. Dump:\n{}",
                           path, data);
                continue;
            }
            if (!description.contains("width")) {
                CORE_ERROR("Texture with path {} is missing texture creation description's 'width' field. It will not be loaded. Dump:\n{}",
                           path, data);
                continue;
            }
            if (!description.contains("height")) {
                CORE_ERROR(
                    "Texture with path {} is missing texture creation description's 'height' field. It will not be loaded. Dump:\n{}", path,
                    data);
                continue;
            }
            if (!description.contains("format")) {
                CORE_ERROR(
                    "Texture with path {} is missing texture creation description's 'format' field. It will not be loaded. Dump:\n{}", path,
                    data);
                continue;
            }
            if (!description.contains("mip_levels")) {
                CORE_ERROR(
                    "Texture with path {} is missing texture creation description's 'mip_levels' field. It will not be loaded. Dump:\n{}",
                    path, data);
                continue;
            }
            if (!description.contains("depth_or_array_size")) {
                CORE_ERROR("Texture with path {} is missing texture creation description's 'depth_or_array_size' field. It will not be "
                           "loaded. Dump:\n{}",
                           path, data);
                continue;
            }
            if (!description.contains("bytes_per_pixel")) {
                CORE_ERROR("Texture with path {} is missing texture creation description's 'bytes_per_pixel' field. It will not be loaded. "
                           "Dump:\n{}",
                           path, data);
                continue;
            }
            sf::render::TextureCreationDesc desc;
            desc.width = description["width"];
            desc.height = description["height"];
            desc.usage = description["usage"];
            desc.path = sf::string_utils::to_wstring(path);
            desc.format = description["format"];
            desc.mip_levels = description["mip_levels"];
            desc.depth_or_array_size = description["depth_or_array_size"];
            if (description.contains("initial_state")) {
                desc.initial_state = description["initial_state"];
            }
            import_texture(device, path, desc, uuid);
        }
    }
    constexpr u32 DEFAULT_TEXTURE_DIMENSIONS = 256;
    constexpr u32 DEFAULT_TEXTURE_CHANNELS = 4;
    constexpr u32 BYTE_COUNT = DEFAULT_TEXTURE_DIMENSIONS * DEFAULT_TEXTURE_DIMENSIONS * DEFAULT_TEXTURE_CHANNELS;
    const sf::render::TextureCreationDesc DEFAULT_TEXTURE_CREATION_INFO = {
        .usage = sf::render::TextureUsage::ShaderResource,
        .format = sf::render::Format::RGBA8_UNORM,
        .width = 256,
        .height = 256,
        .depth_or_array_size = 1,
        .mip_levels = 1,
        .name = L"Default texture",
    };

    static char* default_texture_data() {
        static char data[BYTE_COUNT];
        for (int i = 0; i < BYTE_COUNT; i += 4) {
            data[i] = 255u;
            data[i + 1] = 0u;
            data[i + 2] = 255u;
            data[i + 3] = 255u;
        }
        return data;
    }
    TextureAsset* TextureRegistry::default_texture(sf::render::IGraphicsDevice* device) {
        const static UUID default_texture_uuid = UUID{5596545107579832553};
        static auto data = default_texture_data();
        static TextureAsset asset = {
            .uuid = default_texture_uuid,
            .description = DEFAULT_TEXTURE_CREATION_INFO,
            .data = device->create_texture_with_data(DEFAULT_TEXTURE_CREATION_INFO, data, BYTE_COUNT),
        };
        return &asset;
    }
} // namespace sf::assets
