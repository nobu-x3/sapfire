#include "memory/memory.h"
#include <cassert>
#include <iostream>

namespace sf::mem {
    MemoryManager::MemoryManager(Budgets budgets) {
        m_Backing.resize(budgets.total());
        uint8_t* base = m_Backing.data();
        m_Arenas[static_cast<int32_t>(MemTag::Render)].reset_to(base, budgets.render);
        base += budgets.render;
        m_Arenas[static_cast<int32_t>(MemTag::Logic)].reset_to(base, budgets.logic);
        base += budgets.logic;
        m_Arenas[static_cast<int32_t>(MemTag::Physics)].reset_to(base, budgets.physics);
        base += budgets.physics;
        m_Arenas[static_cast<int32_t>(MemTag::Temp)].reset_to(base, budgets.temp);
        base += budgets.temp;
        m_Arenas[static_cast<int32_t>(MemTag::Strings)].reset_to(base, budgets.strings);
        base += budgets.strings;
        m_Arenas[static_cast<int32_t>(MemTag::Filesystem)].reset_to(base, budgets.fs);
        base += budgets.fs;
        m_Arenas[static_cast<int32_t>(MemTag::Texture)].reset_to(base, budgets.texture);
        base += budgets.texture;
        m_Arenas[static_cast<int32_t>(MemTag::Mesh)].reset_to(base, budgets.mesh);
        base += budgets.mesh;
        m_Arenas[static_cast<int32_t>(MemTag::Material)].reset_to(base, budgets.material);
        base += budgets.material;
        m_Arenas[static_cast<int32_t>(MemTag::Animation)].reset_to(base, budgets.animation);
        base += budgets.animation;
        m_Arenas[static_cast<int32_t>(MemTag::Audio)].reset_to(base, budgets.audio);
        base += budgets.audio;
        m_Arenas[static_cast<int32_t>(MemTag::RTTI)].reset_to(base, budgets.rtti);
        base += budgets.rtti;
        m_Arenas[static_cast<int32_t>(MemTag::Application)].reset_to(base, budgets.application);
        g_instance = this;
    }

    void MemoryManager::report(std::ostream& os) const {
        auto print = [&](MemTag t, const char* name) {
            auto& a = m_Arenas[static_cast<int32_t>(t)];
            os << name << " cap=" << a.capacity() << " used=" << a.used() << " peak=" << a.peak() << " alloc_calls=" << a.alloc_calls()
               << '\n';
        };
        print(MemTag::Render, "Render\t\t");
        print(MemTag::Logic, "Logic\t\t");
        print(MemTag::Physics, "Physics\t\t");
        print(MemTag::Temp, "Temp\t\t");
        print(MemTag::Strings, "Strings\t\t");
        print(MemTag::Filesystem, "FS\t\t");
        print(MemTag::Texture, "Texture\t\t");
        print(MemTag::Mesh, "Mesh\t\t");
        print(MemTag::Material, "Material\t");
        print(MemTag::Animation, "Animation\t");
        print(MemTag::Audio, "Audio\t\t");
        print(MemTag::RTTI, "RTTI\t\t");
    }

    void MemoryManager::reset(MemTag tag) {
        const auto idx = static_cast<size_t>(tag);
        assert(idx < static_cast<size_t>(MemTag::Count));
        m_Arenas[idx].reset();
    }

    void MemoryManager::reset_all() {
        for (auto& a : m_Arenas)
            a.reset();
    }
} // namespace sf::mem
