#include "core/rtti.h"
#include "engpch.h"

#include "components/name_component.h"

namespace sf::components {
    ENGINE_COMPONENT_IMPL(NameComponent);
    std::unordered_map<std::string, u32> g_Names{{"Entity", 0}};
    NameComponent::NameComponent() : m_Name("Entity") {
        if (g_Names.contains(m_Name.c_str())) {
            auto& old_name = m_Name;
            if (g_Names[old_name.c_str()] > 0) {
                std::string temp = std::string(old_name.c_str()) + " (" + std::to_string(g_Names[old_name.c_str()]) + ")";
                m_Name = stl::string(mem::MemTag::Strings, temp);
            }
            g_Names[old_name.c_str()]++;
        } else {
            g_Names[m_Name.c_str()] = 0;
        }
        register_rtti();
    }
    NameComponent::NameComponent(const NameComponent& other) {
        m_Name = other.m_Name;
        register_rtti();
    }
    NameComponent::NameComponent(NameComponent&& other) noexcept {
        m_Name = std::move(other.m_Name);
        m_Rtti = std::move(other.m_Rtti);
        m_Rtti.head = reinterpret_cast<void*>(this);
    }
    NameComponent& NameComponent::operator=(const NameComponent& other) {
        m_Name = other.m_Name;
        register_rtti();
        return *this;
    }
    NameComponent& NameComponent::operator=(NameComponent&& other) noexcept {
        m_Name = std::move(other.m_Name);
        m_Rtti = std::move(other.m_Rtti);
        m_Rtti.head = reinterpret_cast<void*>(this);
        return *this;
    }

    void NameComponent::register_rtti() {
        BEGIN_RTTI()
        ADD_RTTI_FIELD(rtti::rtti_type::STRING, "Name", &m_Name, nullptr)
        END_RTTI();
    }
    NameComponent::NameComponent(stl::string_view name) : m_Name(name) {
        if (g_Names.contains(m_Name.c_str())) {
            auto old_name = m_Name;
            if (g_Names[old_name.c_str()] > 0) {
                std::string temp = std::string(old_name.c_str()) + " (" + std::to_string(g_Names[old_name.c_str()]) + ")";
                m_Name = stl::string(mem::MemTag::Strings, temp);
            }
            g_Names[old_name.c_str()]++;
        } else {
            g_Names[m_Name.c_str()] = 0;
        }
    }
} // namespace sf::components
