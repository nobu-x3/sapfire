#pragma once

#include <iostream>
#include <vector>
#include "core/core.h"
#include "stl/vector.h"
#include "stl/types.h"

namespace sf::stl {

    struct SFAPI generational_index {
        u32 index = 0;
        u32 generation = 0;
    };

    class SFAPI generational_index_allocator {
    public:
        generational_index allocate() {
            if (m_FreeIndices.size() > 0) {
                u32 index = m_FreeIndices.back();
                m_FreeIndices.pop_back();
                m_Entries[index].generation += 1;
                m_Entries[index].is_alive = true;
                return {index, m_Entries[index].generation};
            }
            m_Entries.push_back({true, 0});
            return {static_cast<u32>(m_Entries.size()) - 1, 0};
        }

        void deallocate(generational_index index) {
            if (is_alive(index)) {
                m_Entries[index.index].is_alive = false;
                m_FreeIndices.push_back(index.index);
            }
        }

        bool is_alive(generational_index index) const {
            return index.index < m_Entries.size() && m_Entries[index.index].generation == index.generation &&
                m_Entries[index.index].is_alive;
        }

    private:
        struct entry {
            bool is_alive = false;
            u32 generation = 0;
        };
        std::vector<entry> m_Entries{};
        std::vector<u32> m_FreeIndices{};
    };

    template <typename T>
    class SFAPI generational_vector {
    public:
        struct entry {
            u32 generation;
            T value;
        };

        void set(generational_index index, T val) {
            while (m_Entries.size() <= index.index)
                m_Entries.push_back(std::nullopt);
            u32 prev_gen = 0;
            if (auto prev_entry = m_Entries[index.index])
                prev_gen = prev_entry->generation;
            if (prev_gen > index.generation) {
                std::cerr << "Cannot set value at index" << index.index
                          << ": previous generation is larger than current generation:" << prev_gen << ">" << index.generation << "."
                          << std::endl;
                return;
            }
            m_Entries[index.index] = optional<entry>{{
                .generation = index.generation,
                .value = val,
            }};
        }

        void remove(generational_index index) {
            if (index.index < m_Entries.size()) {
                m_Entries[index.index] = std::nullopt;
            }
        }

        [[nodiscard]] T* get(generational_index index) {
            if (index.index >= m_Entries.size())
                return nullptr;
            if (auto& entry = m_Entries[index.index]) {
                if (entry->generation == index.generation)
                    return &entry->value;
            }
            return nullptr;
        }

        stl::vector<generational_index> get_all_valid_indices(const generational_index_allocator& allocator) const {
            stl::vector<generational_index> result(mem::MemTag::Logic);
            for (u32 i = 0; i < m_Entries.size(); ++i) {
                const auto& entry = m_Entries[i];
                if (!entry)
                    continue;
                generational_index index = {i, entry->generation};
                if (allocator.is_alive(index)) {
                    result.push_back(index);
                }
            }
            return result;
        }

        size_t size() const { return m_Entries.size(); }

        stl::optional<stl::tuple<generational_index, stl::reference_wrapper<const T>>>
        get_first_valid_entry(const generational_index_allocator& allocator) {
            for (auto i = 0; i < m_Entries.size(); ++i) {
                const auto& entry = m_Entries[i];
                if (!entry)
                    continue;
                generational_index index = {i, entry->generation};
                if (allocator.is_alive(index)) {
                    return std::make_tuple(index, std::ref(entry->value));
                }
            }
            return std::nullopt;
        }

        auto begin() { return m_Entries.begin(); }
        auto end() { return m_Entries.end(); }
        auto cbegin() { return m_Entries.cbegin(); }
        auto cend() { return m_Entries.cend(); }
        auto begin() const { return m_Entries.begin(); }
        auto end() const { return m_Entries.end(); }

    private:
        std::vector<stl::optional<entry>> m_Entries;
    };

} // namespace sf::stl
