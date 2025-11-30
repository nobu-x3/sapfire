#pragma once

#include "components/component.h"
#include "components/entity.h"
#include "core/core.h"

namespace sf {
    class SFAPI ECManager {
    public:
        ECManager();
        void reset();
        Entity create_entity(UUID uuid = {});
        void destroy_entity(Entity entity);
        inline bool is_valid(stl::generational_index index) const { return m_EntityRegistry->is_valid(index); }
        inline bool is_valid(Entity entity) const { return m_EntityRegistry->is_valid(entity); }
        inline stl::vector<stl::generational_index> get_all_valid_entities() const { return m_EntityRegistry->get_all_valid_entities(); }
        inline stl::optional<Entity> entity(stl::generational_index index) { return m_EntityRegistry->entity(index); }
        [[nodiscard]] inline const stl::generational_vector<Entity>& entities() const { return m_EntityRegistry->entities(); }

        template <typename T>
        void register_component() {
            m_ComponentRegistry->register_engine_component<T>();
        }

        template <typename T>
        void add_engine_component(Entity entity, T component) {
            m_ComponentRegistry->add_engine_component<T>(entity, component);
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(m_ComponentRegistry->component_type<T>(), true);
            m_EntityRegistry->signature(entity, signature);
        }

        template <typename T>
        void add_engine_component(Entity entity) {
            m_ComponentRegistry->add_engine_component<T>(entity, T());
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(m_ComponentRegistry->component_type<T>(), true);
            m_EntityRegistry->signature(entity, signature);
        }

        template <typename T>
        void remove_engine_component(Entity entity) {
            m_ComponentRegistry->remove_engine_component<T>(entity);
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(m_ComponentRegistry->component_type<T>(), false);
            m_EntityRegistry->signature(entity, signature);
        }

        template <typename T>
        bool has_engine_component(Entity entity) {
            return m_ComponentRegistry->has_engine_component<T>(entity);
        }

        template <typename T, typename K>
        bool get_other_engine_component(const T& first, K& out_other) {
            return m_ComponentRegistry->get_other_engine_component<T, K>(first, out_other);
        }

        template <typename T>
        T& engine_component(Entity entity) {
            return m_ComponentRegistry->get_engine_component<T>(entity);
        }

        template <typename T>
        Entity engine_component_owner(const T& component) {
            return m_ComponentRegistry->get_engine_component_owner<T>(component);
        }

        template <typename T>
        stl::vector<T>& engine_components() {
            return m_ComponentRegistry->engine_components<T>();
        }

        void add_component(Entity entity, stl::shared_ptr<components::IComponent> component) {
            m_ComponentRegistry->add_component(entity, component);
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(component->component_type(), true);
            m_EntityRegistry->signature(entity, signature);
        }

        void add_component(Entity entity, components::ComponentType type) {
            m_ComponentRegistry->add_component(entity, type);
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(type, true);
            m_EntityRegistry->signature(entity, signature);
        }

        void remove_component(Entity entity, stl::shared_ptr<components::IComponent> component) {
            m_ComponentRegistry->remove_component(entity, component);
            auto signature = m_EntityRegistry->signature(entity);
            signature.set(component->component_type(), false);
            m_EntityRegistry->signature(entity, signature);
        }

        stl::shared_ptr<components::IComponent> component(Entity entity, const char* name) {
            return m_ComponentRegistry->component(entity, name);
        }

        stl::shared_ptr<components::IComponent> component(Entity entity, components::ComponentType type) {
            return m_ComponentRegistry->component(entity, type);
        }

        stl::vector<stl::shared_ptr<components::IComponent>> components(Entity entity) {
            return m_ComponentRegistry->components(entity, m_EntityRegistry->signature(entity));
        }

        // Runtime operations by component type name
        bool has_component(Entity entity, const stl::string& type_name) { return m_ComponentRegistry->has_component(entity, type_name); }
        void add_component(Entity entity, const stl::string& type_name) {
            m_ComponentRegistry->add_component(entity, type_name);
            auto signature = m_EntityRegistry->signature(entity);
            std::string key = type_name.c_str();
            auto it = ::sf::components::ComponentRegistry::s_ComponentTypes.find(key);
            if (it != ::sf::components::ComponentRegistry::s_ComponentTypes.end())
                signature.set(it->second, true);
            m_EntityRegistry->signature(entity, signature);
        }

        void reset_component(Entity entity, const stl::string& type_name) {
            m_ComponentRegistry->reset_component(entity, type_name);
        }

        ::sf::rtti::rtti_object* rtti_for(Entity entity, const stl::string& type_name) { return m_ComponentRegistry->rtti_for(entity, type_name); }

    private:
        stl::unique_ptr<components::ComponentRegistry> m_ComponentRegistry;
        stl::unique_ptr<EntityRegistry> m_EntityRegistry;
    };
}; // namespace sf
