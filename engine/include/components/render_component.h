#pragma once

#include "components/component.h"

namespace sf {
    namespace d3d {
        class GraphicsDevice;
    }
    namespace assets {
        class MeshRegistry;
    }
} // namespace sf

namespace sf::components {

    struct PerDrawConstants {
        sf::u32 position_buffer_idx = 0;
        sf::u32 normal_buffer_idx = 0;
        sf::u32 tangent_buffer_idx = 0;
        sf::u32 uv_buffer_idx = 0;
        sf::u32 scene_cbuffer_idx = 0;
        sf::u32 pass_cbuffer_idx = 0;
        sf::u32 material_cbuffer_idx = 0;
        sf::u32 texture_cbuffer_idx = 0;
    };

    struct CPUData {
        sf::u32 indices_size = 0;
        sf::u32 index_id = 0;
        sf::u32 position_idx = 0;
        sf::u32 normal_idx = 0;
        sf::u32 tangent_idx = 0;
        sf::u32 uv_idx = 0;
        sf::u32 transform_buffer_idx = 0;
    };

    class RenderComponent {
        RTTI;
        ENGINE_COMPONENT(RenderComponent);

    public:
        RenderComponent();
        RenderComponent(const RenderComponent& other);
        RenderComponent(RenderComponent&& other) noexcept;
        RenderComponent& operator=(const RenderComponent& other);
        RenderComponent& operator=(RenderComponent&& other) noexcept;
        RenderComponent(UUID mesh_uuid, UUID texture_uuid, UUID material_uuid, CPUData cpu_data, PerDrawConstants per_draw_constants,
                        stl::function<void(RenderComponent*)> = nullptr);
        bool operator==(const RenderComponent& other) const;
        void register_rtti();

        inline PerDrawConstants* per_draw_constants() { return &m_PerDrawConstants; }
        inline void per_draw_constants(const PerDrawConstants& constants) { m_PerDrawConstants = constants; }
        inline CPUData cpu_data() const { return m_CPUData; }
        inline void cpu_data(CPUData data) { m_CPUData = data; }
        inline UUID mesh_uuid() const { return m_MeshUUID; }
        inline UUID material_uuid() const { return m_MaterialUUID; }
        inline UUID texture_uuid() const { return m_TextureUUID; }

    private:
        UUID m_MeshUUID;
        UUID m_MaterialUUID;
        UUID m_TextureUUID;
        CPUData m_CPUData{};
        PerDrawConstants m_PerDrawConstants{};
        std::function<void(RenderComponent*)> m_OptionalSetter = nullptr;
    };
} // namespace sf::components
