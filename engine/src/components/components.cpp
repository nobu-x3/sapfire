#include "core/core.h"
#include "engpch.h"

#include "components/component.h"

namespace sf::components {

    std::unordered_map<std::string, ComponentType> ComponentRegistry::s_ComponentTypes = {};
    std::unordered_map<ComponentType, std::string> ComponentRegistry::s_ComponentTypeNameMap = {};
    std::unordered_map<std::string, std::shared_ptr<IComponentList>> ComponentRegistry::s_EngineComponentLists = {};
    std::unordered_map<std::string, std::shared_ptr<CustomComponentList>> ComponentRegistry::s_CustomComponentLists{};
    ComponentType ComponentRegistry::s_NextComponentTypeNumber = 0;

    // Track live ComponentRegistry instances so shutdown() can clear their member maps
    static std::vector<ComponentRegistry*>& get_instance_list() {
        static std::vector<ComponentRegistry*> instances;
        return instances;
    }

    // Two queues: one for registration functions, one for actual registrations
    static std::vector<std::function<void()>>& get_registration_func_list() {
        static std::vector<std::function<void()>> funcs;
        return funcs;
    }

    static std::vector<std::function<void()>>& get_registration_queue() {
        static std::vector<std::function<void()>> queue;
        return queue;
    }

    void ComponentRegistry::register_component_registration_func(std::function<void()> func) {
        get_registration_func_list().push_back(func);
    }

    void ComponentRegistry::queue_registration(std::function<void()> registration_func) {
        get_registration_queue().push_back(registration_func);
    }

    void ComponentRegistry::shutdown() {
        s_ComponentTypes.clear();
        s_ComponentTypeNameMap.clear();
        s_EngineComponentLists.clear();
        s_CustomComponentLists.clear();
        for (auto* inst : get_instance_list()) {
            inst->m_EngineComponentLists.clear();
            inst->m_CustomComponentLists.clear();
            inst->m_ComponentTypes.clear();
            inst->m_ComponentTypeNameMap.clear();
            inst->m_NextComponentTypeNumber = 0;
        }
    }

    void ComponentRegistry::process_queued_registrations() {
        for (auto& func : get_registration_func_list()) {
            func();
        }
        get_registration_func_list().clear();
        auto& queue = get_registration_queue();
        for (auto& func : queue) {
            func();
        }
        queue.clear();
    }

    ComponentRegistry::ComponentRegistry() :
        m_ComponentTypes(mem::MemTag::Logic), m_ComponentTypeNameMap(mem::MemTag::Logic), m_EngineComponentLists(mem::MemTag::Logic),
        m_CustomComponentLists(mem::MemTag::Logic) {
        // Copy from static members to instance members
        // Cannot use copy constructors due to different allocators
        for (const auto& [key, value] : s_ComponentTypes) {
            m_ComponentTypes.emplace(stl::string(mem::MemTag::Strings, key.c_str()), value);
        }
        for (const auto& [key, value] : s_ComponentTypeNameMap) {
            m_ComponentTypeNameMap.emplace(key, stl::string(mem::MemTag::Strings, value.c_str()));
        }
        for (const auto& [key, value] : s_EngineComponentLists) {
            m_EngineComponentLists.emplace(stl::string(mem::MemTag::Strings, key.c_str()), value);
        }
        for (const auto& [key, value] : s_CustomComponentLists) {
            m_CustomComponentLists.emplace(stl::string(mem::MemTag::Strings, key.c_str()), value);
        }
        m_NextComponentTypeNumber = s_NextComponentTypeNumber;
        // register this instance so shutdown() can clear it
        get_instance_list().push_back(this);
    }

    ComponentRegistry::~ComponentRegistry() {
        auto& list = get_instance_list();
        auto it = std::find(list.begin(), list.end(), this);
        if (it != list.end())
            list.erase(it);
        m_EngineComponentLists.clear();
        m_CustomComponentLists.clear();
        m_ComponentTypes.clear();
        m_ComponentTypeNameMap.clear();
    }

    CustomComponentList::CustomComponentList(const stl::shared_ptr<IComponent>& def_comp) : default_component(def_comp) {}
    stl::string CustomComponentList::to_tstring() { return default_component->to_string(); }

    void CustomComponentList::insert(Entity entity, stl::shared_ptr<IComponent>& component) {
        if (m_EntityToIndexMap.size() > 0 && m_EntityToIndexMap.contains(entity)) {
            remove(entity);
        }
        m_EntityToIndexMap[entity] = m_Components.size();
        m_IndexToEntityMap[m_Components.size()] = entity;
        m_Components.push_back(component);
    }

    void CustomComponentList::remove(Entity entity) {
        if (m_Components.size() <= 0)
            return;
        // swap element at end to deleted element's place to maintain density
        auto removed_entity_index = m_EntityToIndexMap[entity];
        auto index_of_last_entity = m_Components.size() - 1;
        const Entity last_entity = m_IndexToEntityMap[index_of_last_entity];
        std::swap(m_Components[removed_entity_index], m_Components[m_Components.size() - 1]);
        // update maps
        m_EntityToIndexMap[last_entity] = removed_entity_index;
        m_IndexToEntityMap[removed_entity_index] = last_entity;
        m_EntityToIndexMap.erase(entity);
        m_IndexToEntityMap.erase(index_of_last_entity);
        m_Components.pop_back();
    }
    stl::shared_ptr<IComponent> CustomComponentList::get(Entity entity) { return m_Components[m_EntityToIndexMap[entity]]; }

    void CustomComponentList::entity_destroyed(Entity entity) {
        if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end()) {
            remove(entity);
        }
    }

    void ComponentRegistry::add_component(Entity entity, stl::shared_ptr<IComponent>& component) {
        const stl::string& type_name = m_ComponentTypeNameMap[component->component_type()];
        stl::shared_ptr<CustomComponentList>& component_list = m_CustomComponentLists[type_name];
        if (!component_list)
            component_list = stl::make_shared<CustomComponentList>(mem::MemTag::Logic);
        component_list->insert(entity, component);
    }

    void ComponentRegistry::add_component(Entity entity, ComponentType component_type) {
        const stl::string& type_name = m_ComponentTypeNameMap[component_type];
        stl::shared_ptr<CustomComponentList>& component_list = m_CustomComponentLists[type_name];
        if (!component_list)
            component_list = stl::make_shared<CustomComponentList>(mem::MemTag::Logic);
        stl::shared_ptr<IComponent> component;
        component_list->default_component->copy(component);
        component_list->insert(entity, component);
    }

    void ComponentRegistry::remove_component(Entity entity, stl::shared_ptr<IComponent>& component) {
        const stl::string& type_name = m_ComponentTypeNameMap[component->component_type()];
        const stl::shared_ptr<CustomComponentList>& component_list = m_CustomComponentLists[type_name];
        component_list->remove(entity);
    }

    stl::shared_ptr<IComponent> ComponentRegistry::component(Entity entity, ComponentType type) {
        const stl::string& type_name = m_ComponentTypeNameMap[type];
        const stl::shared_ptr<CustomComponentList>& component_list = m_CustomComponentLists[type_name];
        return component_list->get(entity);
    }

    stl::shared_ptr<IComponent> ComponentRegistry::component(Entity entity, const char* type_name) {
        const stl::shared_ptr<CustomComponentList>& component_list = m_CustomComponentLists[type_name];
        return component_list->get(entity);
    }

    stl::vector<stl::shared_ptr<IComponent>> ComponentRegistry::components(Entity entity, Signature signature) {
        stl::vector<stl::shared_ptr<IComponent>> return_vector{};
        for (int i = 0; i < signature.size(); ++i) {
            if (!signature[i])
                continue;
            const stl::string& component_name = m_ComponentTypeNameMap[static_cast<ComponentType>(i)];
            const stl::shared_ptr<CustomComponentList> component_list = m_CustomComponentLists[component_name];
            if (component_list)
                return_vector.push_back(component_list->get(entity));
        }
        return return_vector;
    }

    void ComponentRegistry::entity_destroyed(Entity entity) {
        for (auto& [name, component_list] : m_EngineComponentLists) {
            component_list->entity_destroyed(entity);
        }
        for (auto& [_, component_list] : m_CustomComponentLists) {
            if (component_list)
                component_list->entity_destroyed(entity);
        }
    }
} // namespace sf::components
