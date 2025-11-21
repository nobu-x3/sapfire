#pragma once

#include "Sapfire.h"

namespace assets {
	class ProjectReader {
	public:
		explicit ProjectReader(sf::assets::AssetManager* asset_manager, sf::stl::string& project_name);
		void serialize(const sf::stl::string& project_path);
		void deserealize(const sf::stl::string& project_path);

	private:
		sf::assets::AssetManager& m_AssetManager;
		sf::stl::string& m_ProjectName;
	};
} // namespace assets