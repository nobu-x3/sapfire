#include "Sapfire.h"
#include "imgui.h"

namespace icons {
    constexpr const char* MESH_ICON_64_ID = "mesh_icon_64";
    constexpr const char* MESH_ICON_16_ID = "mesh_icon_16";
	constexpr const char* IMAGE_ICON_64_ID = "image_icon_64";
	constexpr const char* IMAGE_ICON_16_ID = "image_icon_16";

    const Sapfire::sf::render::Texture& get(const Sapfire::stl::string& id);
    ImTextureID get_im_id(const Sapfire::stl::string& id);
    void add(Sapfire::render::IGraphicsDevice* device, const Sapfire::stl::wstring& path, const Sapfire::stl::string& id);
}
