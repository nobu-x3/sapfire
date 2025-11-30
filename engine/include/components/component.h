#pragma once

#include <memory>
#include "components/entity.h"
#include "core/core.h"
#include "core/logger.h"
#include "core/rtti.h"
#include "memory/memory.h"

namespace sf::components {

    using ComponentType = u8;

    class IComponent {
    public:
        virtual ~IComponent() = default;
        virtual void update(f32 delta_time) = 0;
        virtual stl::string to_string() const = 0;
        virtual rtti::rtti_object& get_rtti() = 0;
        virtual ComponentType component_type() = 0;
        virtual void copy(stl::shared_ptr<IComponent>&) = 0;
    };

    class IComponentList {
    public:
        virtual ~IComponentList() = default;
        virtual void entity_destroyed(Entity entity) = 0;
        virtual stl::string to_tstring() = 0;
    };

    template <typename T>
    class EngineComponentList : public IComponentList {
    public:
        stl::string to_tstring() override { return T::to_tstring(); }

        void insert(Entity entity, T component) {
            if (m_EntityToIndexMap.contains(entity)) {
                auto index = m_EntityToIndexMap[entity];
                m_Components[index] = component;
                /* remove(entity); */
                return;
            }
            m_EntityToIndexMap[entity] = m_Components.size();
            m_IndexToEntityMap[m_Components.size()] = entity;
            m_Components.push_back(component);
        }

        bool exists(Entity entity) { return m_EntityToIndexMap.contains(entity); }

        void remove(Entity entity) {
            if (m_Components.size() <= 0)
                return;
            // swap element at end to deleted element's place to maintain density
            auto removed_entity_index = m_EntityToIndexMap[entity];
            auto index_of_last_entity = m_Components.size() - 1;
            Entity last_entity = m_IndexToEntityMap[index_of_last_entity];
            std::swap(m_Components[removed_entity_index], m_Components[m_Components.size() - 1]);
            // update maps
            m_EntityToIndexMap[last_entity] = removed_entity_index;
            m_IndexToEntityMap[removed_entity_index] = last_entity;
            m_EntityToIndexMap.erase(entity);
            m_IndexToEntityMap.erase(index_of_last_entity);
            m_Components.pop_back();
        }

        T& get(Entity entity) { return m_Components[m_EntityToIndexMap[entity]]; }

        void entity_destroyed(Entity entity) override {
            if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end()) {
                remove(entity);
            }
        }

        stl::vector<T>& components() { return m_Components; }

        Entity entity(size_t index) { return m_IndexToEntityMap[index]; }

        Entity get_owner(const T& component) {
            // We assume the give component is in the list
            size_t index = m_Components.size();
            for (u32 i = 0; i < m_Components.size(); ++i) {
                if (m_Components[i].uuid() == component.uuid()) {
                    index = i;
                    break;
                }
            }
            // Verify component is registered
            if (index >= m_Components.size()) {
                CORE_ERROR("get_owner() called on unregistered component");
                return Entity{}; // Return invalid entity
            }
            return m_IndexToEntityMap[index];
        }

    private:
        stl::vector<T> m_Components{mem::MemTag::Logic};
        stl::unordered_map<Entity, size_t> m_EntityToIndexMap{mem::MemTag::Logic};
        stl::unordered_map<size_t, Entity> m_IndexToEntityMap{mem::MemTag::Logic};
    };

    class CustomComponentList : public IComponentList {

    public:
        CustomComponentList() = default;
        CustomComponentList(const stl::shared_ptr<IComponent>& def_comp);
        CustomComponentList(const CustomComponentList&) = default;
        CustomComponentList(CustomComponentList&&) noexcept = default;
        CustomComponentList& operator=(const CustomComponentList&) = default;
        CustomComponentList& operator=(CustomComponentList&&) noexcept = default;
        stl::shared_ptr<IComponent> default_component;
        stl::string to_tstring() override;
        void insert(Entity entity, stl::shared_ptr<IComponent>& component);
        void remove(Entity entity);
        stl::shared_ptr<IComponent> get(Entity entity);
        void entity_destroyed(Entity entity) override;
        const stl::vector<stl::shared_ptr<IComponent>>& components() const { return m_Components; }

    private:
        stl::vector<stl::shared_ptr<IComponent>> m_Components{mem::MemTag::Logic};
        stl::unordered_map<Entity, size_t> m_EntityToIndexMap{mem::MemTag::Logic};
        stl::unordered_map<size_t, Entity> m_IndexToEntityMap{mem::MemTag::Logic};
    };

    class ComponentRegistry {
    public:
        ComponentRegistry();
        ~ComponentRegistry();

        // Helper: Create a registration lambda that properly captures type info at definition time
        template <typename T>
        static std::function<void()> make_engine_registration_lambda() {
            const char* type_name = typeid(T).name();
            return [type_name]() {
                s_ComponentTypes[type_name] = s_NextComponentTypeNumber;
                s_ComponentTypeNameMap[s_NextComponentTypeNumber] = type_name;
                s_EngineComponentLists[type_name] = std::make_shared<EngineComponentList<T>>();
                s_NextComponentTypeNumber++;
            };
        }

        template <typename T>
        static void queue_engine_component_registration() {
            queue_registration(make_engine_registration_lambda<T>());
        }

        static void queue_custom_component_registration(std::function<stl::shared_ptr<IComponent>()> factory) {
            queue_registration([factory]() {
                auto component = factory();
                auto type_str = component->to_string();
                const char* type_name = type_str.c_str();
                s_ComponentTypes[type_name] = s_NextComponentTypeNumber;
                s_ComponentTypeNameMap[s_NextComponentTypeNumber] = type_name;
                s_CustomComponentLists[type_name] = std::make_shared<CustomComponentList>(component);
                s_NextComponentTypeNumber++;
            });
        }

        static void process_queued_registrations();

        // Internal: Register a component registration function to be called during process_queued_registrations
        static void register_component_registration_func(std::function<void()> func);

        static void shutdown();

    private:
        static void register_instance(ComponentRegistry* inst);
        static void unregister_instance(ComponentRegistry* inst);
        static void queue_registration(std::function<void()> registration_func);

    public:
        template <typename T>
        void register_engine_component() {
            const char* type_name = typeid(T).name();
            m_ComponentTypes[type_name] = m_NextComponentTypeNumber;
            m_ComponentTypeNameMap[m_NextComponentTypeNumber] = type_name;
            m_EngineComponentLists[type_name] = stl::make_shared<EngineComponentList<T>>(mem::MemTag::Logic);
            m_NextComponentTypeNumber++;
        }

        template <typename T>
        ComponentType component_type() {
            const char* type_name = typeid(T).name();
            return m_ComponentTypes[type_name];
        }

        template <typename T>
        bool has_engine_component(Entity entity) {
            const char* type_name = typeid(T).name();
            return m_EngineComponentLists.contains(type_name) &&
                std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->exists(entity);
        }

        template <typename T>
        void add_engine_component(Entity entity, T component) {
            const char* type_name = typeid(T).name();
            std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->insert(entity, component);
        }

        template <typename T>
        void remove_engine_component(Entity entity) {
            const char* type_name = typeid(T).name();
            std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->remove(entity);
        }

        template <typename T>
        T& get_engine_component(Entity entity) {
            const char* type_name = typeid(T).name();
            return std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->get(entity);
        }

        template <typename T>
        Entity get_engine_component_owner(const T& component) {
            const char* type_name = typeid(T).name();
            return std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->get_owner(component);
        }

        template <typename T>
        stl::vector<T>& engine_components() {
            const char* type_name = typeid(T).name();
            return std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[type_name])->components();
        }

        template <typename T, typename K>
        bool get_other_engine_component(const T& first, K& out_other) {
            const char* first_type_name = typeid(T).name();
            const char* second_type_name = typeid(K).name();
            if (!m_EngineComponentLists.contains(first_type_name) || !m_EngineComponentLists.contains(second_type_name))
                return false;
            auto first_component_list = std::static_pointer_cast<EngineComponentList<T>>(m_EngineComponentLists[first_type_name]);
            stl::vector<T>& first_components = first_component_list->components();
            const auto found_it = std::find(first_components.begin(), first_components.end(), first);
            if (found_it == first_components.end()) {
                return false;
            }
            size_t first_index = std::distance(first_components.begin(), found_it);
            Entity entity = first_component_list->entity(first_index);
            if (!has_engine_component<K>(entity))
                return false;
            out_other = get_engine_component<K>(entity);
            return true;
        }

        void add_component(Entity entity, stl::shared_ptr<IComponent>& component);
        void add_component(Entity entity, ComponentType component_type);
        void remove_component(Entity entity, stl::shared_ptr<IComponent>& component);
        stl::shared_ptr<IComponent> component(Entity entity, ComponentType type);
        stl::shared_ptr<IComponent> component(Entity entity, const char* type_name);
        stl::vector<stl::shared_ptr<IComponent>> components(Entity entity, Signature signature);
        void entity_destroyed(Entity entity);

    private:
        stl::unordered_map<stl::string, ComponentType> m_ComponentTypes;
        stl::unordered_map<ComponentType, stl::string> m_ComponentTypeNameMap;
        stl::unordered_map<stl::string, std::shared_ptr<IComponentList>> m_EngineComponentLists;
        stl::unordered_map<stl::string, std::shared_ptr<CustomComponentList>> m_CustomComponentLists;
        ComponentType m_NextComponentTypeNumber{};

    public:
        static std::unordered_map<std::string, ComponentType> s_ComponentTypes;
        static std::unordered_map<ComponentType, std::string> s_ComponentTypeNameMap;
        static std::unordered_map<std::string, std::shared_ptr<IComponentList>> s_EngineComponentLists;
        static std::unordered_map<std::string, std::shared_ptr<CustomComponentList>> s_CustomComponentLists;
        static ComponentType s_NextComponentTypeNumber;
    };

#define COMPONENT(type)                                                                                                                    \
public:                                                                                                                                    \
    inline ::sf::stl::string to_string() const override { return ::sf::stl::string(::sf::mem::MemTag::Strings, s_ComponentName); }         \
    inline ::sf::components::ComponentType component_type() override { return s_ComponentType; }                                           \
    inline void copy(::sf::stl::shared_ptr<IComponent>& dest) override {                                                                   \
        dest = ::sf::stl::make_shared<type>(::sf::mem::MemTag::Logic, *this);                                                              \
    }                                                                                                                                      \
                                                                                                                                           \
private:                                                                                                                                   \
    static const char* s_ComponentName;                                                                                                    \
    static ::sf::components::ComponentType s_ComponentType;

#define COMPONENT_IMPL(type)                                                                                                               \
    const char* type::s_ComponentName = #type;                                                                                             \
    ::sf::components::ComponentType type::s_ComponentType = 0; /* Will be set during registration */                                       \
    void register_custom_component_##type() {                                                                                              \
        ::sf::components::ComponentRegistry::queue_custom_component_registration(                                                          \
            []() { return ::sf::stl::make_shared<type>(::sf::mem::MemTag::Logic); });                                                      \
    }                                                                                                                                      \
    struct RegisterCustomComponent##type {                                                                                                 \
        RegisterCustomComponent##type() {                                                                                                  \
            ::sf::components::ComponentRegistry::register_component_registration_func(register_custom_component_##type);                   \
        }                                                                                                                                  \
    };                                                                                                                                     \
    inline RegisterCustomComponent##type _register_custom_component_##type;

#define ENGINE_COMPONENT_IMPL(type)                                                                                                        \
    void register_engine_component_##type() { ::sf::components::ComponentRegistry::queue_engine_component_registration<type>(); }          \
    struct RegisterEngineComponent##type {                                                                                                 \
        RegisterEngineComponent##type() {                                                                                                  \
            ::sf::components::ComponentRegistry::register_component_registration_func(register_engine_component_##type);                   \
        }                                                                                                                                  \
    };                                                                                                                                     \
    inline RegisterEngineComponent##type _register_engine_component_##type;

#define ENGINE_COMPONENT(type)                                                                                                             \
public:                                                                                                                                    \
    static ::sf::stl::string to_tstring() { return #type; }                                                                                \
                                                                                                                                           \
private:
} // namespace sf::components
