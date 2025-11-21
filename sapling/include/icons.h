#include "Sapfire.h"
#include "imgui.h"

namespace icons {
    constexpr const char* MESH_ICON_64_ID = "mesh_icon_64";
    constexpr const char* MESH_ICON_16_ID = "mesh_icon_16";
	constexpr const char* IMAGE_ICON_64_ID = "image_icon_64";
	constexpr const char* IMAGE_ICON_16_ID = "image_icon_16";

    const sf::render::Texture& get(const sf::stl::string& id);
    ImTextureID get_im_id(const sf::stl::string& id);
    void add(sf::render::IGraphicsDevice* device, const sf::stl::wstring& path, const sf::stl::string& id);
}
