#pragma once

#include "components/entity.h"
namespace sf {
    class ECManager;
    struct RenderComponentResourcePaths;
    namespace assets {
        class AssetManager;
    } // namespace assets
} // namespace sf

namespace sf::assets {
    class SFAPI SceneWriter {
    public:
        explicit SceneWriter(ECManager* ec, AssetManager* am);
        void serialize(const stl::string& scene_path);
        void
        deserealize(const stl::string& scene_path,
                    stl::function<void(sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths)> render_component_setter);

    private:
        ECManager& m_ECManager;
        AssetManager& m_AssetManager;
    };
} // namespace sf::assets
