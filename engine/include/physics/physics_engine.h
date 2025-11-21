#pragma once

#include "core/core.h"

namespace sf {
    class ECManager;
}

namespace sf::physics {

	class SFAPI PhysicsEngine {
	public:
        PhysicsEngine(ECManager* ec_manager);
		void simulate(f32 delta_time);

	private:
        ECManager& m_ECManager;
	};
} // namespace sf::physics
