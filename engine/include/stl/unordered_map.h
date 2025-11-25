#pragma once

#include <cassert>
#include <functional>
#include <unordered_map>
#include "memory/memory.h"
#include "stl/string.h"
#include "core/core.h"

namespace sf::stl {

    // String hash and equality for transparent lookup
    struct TStringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
    };

    struct TStringEqual {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const { return a == b; }
    };

    template <class K, class V>
    class unordered_map : public std::unordered_map<K, V, std::conditional_t<std::is_same_v<K, string>, TStringHash, std::hash<K>>,
                                                    std::conditional_t<std::is_same_v<K, string>, TStringEqual, std::equal_to<K>>,
                                                    mem::PoolTaggedAllocator<std::pair<const K, V>>> {
    public:
        using base_type = std::unordered_map<K, V, std::conditional_t<std::is_same_v<K, string>, TStringHash, std::hash<K>>,
                                             std::conditional_t<std::is_same_v<K, string>, TStringEqual, std::equal_to<K>>,
                                             mem::PoolTaggedAllocator<std::pair<const K, V>>>;
        using allocator_type = mem::PoolTaggedAllocator<std::pair<const K, V>>;
        using hasher = std::conditional_t<std::is_same_v<K, string>, TStringHash, std::hash<K>>;
        using key_equal = std::conditional_t<std::is_same_v<K, string>, TStringEqual, std::equal_to<K>>;

        explicit unordered_map(mem::MemTag tag) :
            base_type(0, hasher{}, key_equal{}, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        unordered_map(mem::MemTag tag, typename base_type::size_type bucket_count) :
            base_type(bucket_count, hasher{}, key_equal{}, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        unordered_map(const unordered_map& other) : base_type(other) {}

        unordered_map(unordered_map&& other) noexcept : base_type(std::move(other)) {}

        unordered_map& operator=(const unordered_map& other) {
            base_type::operator=(other);
            return *this;
        }

        unordered_map& operator=(unordered_map&& other) noexcept {
            base_type::operator=(std::move(other));
            return *this;
        }

        using base_type::base_type;
    };

    template <class K, class V>
    [[nodiscard]] inline unordered_map<K, V> make_unordered_map(mem::MemTag tag) {
        auto& mm = *mem::MemoryManager::get();
        assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        return unordered_map<K, V>(0, std::conditional_t<std::is_same_v<K, string>, TStringHash, std::hash<K>>{},
                                   std::conditional_t<std::is_same_v<K, string>, TStringEqual, std::equal_to<K>>{},
                                   mem::PoolTaggedAllocator<std::pair<const K, V>>(&mm.arena(tag)));
    }

} // namespace sf::stl
