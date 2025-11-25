#pragma once

#include <array>
#include <bitset>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include "memory/memory.h"

// Platform-specific DLL export/import
#ifdef SF_PLATFORM_WINDOWS
#ifdef SF_BUILD_DLL
#define SFAPI __declspec(dllexport)
#else
#define SFAPI __declspec(dllimport)
#endif
#else
// Linux/macOS: use visibility attributes for shared libraries
#ifdef SF_BUILD_DLL
#define SFAPI __attribute__((visibility("default")))
#else
#define SFAPI
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif

namespace sf {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    using f32 = float;
    using f64 = double;
    using RendererID = u32;

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

#define BIND_EVENT_FN_FOR_OBJ(o, x) std::bind(&x, o, std::placeholders::_1)

    namespace stl {
        // Standard STL type aliases (non-allocating types only)
        template <class _This, class... _Rest>
        using tuple = std::tuple<_This, _Rest...>;
        template <class _Ty>
        using reference_wrapper = std::reference_wrapper<_Ty>;
        template <typename _Ty, class _Container = std::deque<_Ty>>
        using queue = std::queue<_Ty, _Container>;

        template <typename T>
        using function = std::function<T>;
        using string_view = std::string_view;
        using mutex = std::mutex;
        template <typename Mutex>
        using lock_guard = std::lock_guard<Mutex>;
        template <typename Mutex>
        using unique_lock = std::unique_lock<Mutex>;
        template <typename Mutex>
        using scoped_lock = std::scoped_lock<Mutex>;
        using thread = std::thread;
        using jthread = std::jthread;
        template <typename T>
        using optional = std::optional<T>;
        template <typename T, std::size_t Extent = std::dynamic_extent>
        using span = std::span<T, Extent>;
        template <class _Ty, size_t _Size>
        using array = std::array<_Ty, _Size>;
        using wstring_view = std::wstring_view;
        using recursive_mutex = std::recursive_mutex;
        template <size_t Size>
        using bitset = std::bitset<Size>;

        // Tagged allocator versions (using underworld memory system)
        // These use LinearTaggedAllocator for vectors and strings
        template <class T>
        class vector : public std::vector<T, mem::LinearTaggedAllocator<T>> {
        public:
            using base_type = std::vector<T, mem::LinearTaggedAllocator<T>>;
            using allocator_type = mem::LinearTaggedAllocator<T>;

            explicit vector(mem::MemTag tag) : base_type(allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            vector(mem::MemTag tag, typename base_type::size_type count) :
                base_type(count, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            vector(mem::MemTag tag, typename base_type::size_type count, const T& value) :
                base_type(count, value, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            vector(const vector& other) : base_type(other) {}

            vector(vector&& other) noexcept : base_type(std::move(other)) {}

            vector& operator=(const vector& other) {
                base_type::operator=(other);
                return *this;
            }

            vector& operator=(vector&& other) noexcept {
                base_type::operator=(std::move(other));
                return *this;
            }

            using base_type::base_type;
        };

        template <typename T>
        using unique_ptr = std::unique_ptr<T, TaggedDeleter<T>>;

        template <typename T, typename... Args>
        unique_ptr<T> make_unique(mem::MemTag tag, Args&&... args) {
            auto* mm = mem::MemoryManager::get();
            assert(mm && "MemoryManager not initialized");

            auto& arena = mm->arena(tag);
            void* mem = arena.allocate(sizeof(T), alignof(T));
            T* ptr = new (mem) T(std::forward<Args>(args)...);

            return unique_ptr<T>(ptr, TaggedDeleter<T>(tag));
        }

        template <typename T>
        using shared_ptr = std::shared_ptr<T>;

        template <typename T, typename... Args>
        shared_ptr<T> make_shared(mem::MemTag tag, Args&&... args) {
            auto* mm = mem::MemoryManager::get();
            assert(mm && "MemoryManager not initialized");

            // Use allocate_shared with custom allocator
            return std::allocate_shared<T>(mem::LinearTaggedAllocator<T>(&mm->arena(tag)), std::forward<Args>(args)...);
        }

        // String hash and equality for transparent lookup
        struct TStringHash {
            using is_transparent = void;
            size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
        };

        struct TStringEqual {
            using is_transparent = void;
            bool operator()(std::string_view a, std::string_view b) const { return a == b; }
        };

        class string : public std::basic_string<char, std::char_traits<char>, mem::LinearTaggedAllocator<char>> {
        public:
            using base_type = std::basic_string<char, std::char_traits<char>, mem::LinearTaggedAllocator<char>>;
            using allocator_type = mem::LinearTaggedAllocator<char>;

            explicit string(mem::MemTag tag) : base_type(allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string() : base_type(allocator_type(&mem::MemoryManager::get()->arena(mem::MemTag::Strings))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string(mem::MemTag tag, const char* str) : base_type(str, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string(mem::MemTag tag, const std::string& str) :
                base_type(str.c_str(), allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string(mem::MemTag tag, std::string_view sv) :
                base_type(sv.data(), sv.size(), allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string(const string& other) : base_type(other) {}

            string(const base_type& other) : base_type(other) {}

            string(string&& other) noexcept : base_type(std::move(other)) {}

            string(base_type&& other) noexcept : base_type(std::move(other)) {}

            string(const char* str) : base_type(str, allocator_type(&mem::MemoryManager::get()->arena(mem::MemTag::Strings))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string(std::string_view sv) :
                base_type(sv.data(), sv.size(), allocator_type(&mem::MemoryManager::get()->arena(mem::MemTag::Strings))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            string& operator=(const string& other) {
                base_type::operator=(other);
                return *this;
            }

            string& operator=(string&& other) noexcept {
                base_type::operator=(std::move(other));
                return *this;
            }

            string& operator=(const char* str) {
                base_type::operator=(str);
                return *this;
            }

            string& operator=(const std::string& str) {
                base_type::operator=(str.c_str());
                return *this;
            }

            string& operator=(const base_type& other) {
                base_type::operator=(other);
                return *this;
            }

            string& operator=(std::string_view sv) {
                base_type::assign(sv.data(), sv.size());
                return *this;
            }

            using base_type::base_type;
        };

        using tstringstream = std::basic_stringstream<char, std::char_traits<char>, mem::LinearTaggedAllocator<char>>;

        template <typename... Args>
        [[nodiscard]] string make_string(mem::MemTag tag, Args&&... args) {
            auto* mm = mem::MemoryManager::get();
            assert(mm);
            if (!mm) {
                throw std::runtime_error("Memory Manager not initialized.");
            }
            auto* arena = &mm->arena(tag);
            assert(arena);
            return string(std::forward<Args>(args)..., mem::LinearTaggedAllocator<char>(arena));
        }

        using tmp_string = std::basic_string<char, std::char_traits<char>, mem::TempAllocator<char>>;

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

        // Tagged map using PoolTaggedAllocator
        template <class Key, class T, class Compare = std::less<Key>>
        class map : public std::map<Key, T, Compare, mem::PoolTaggedAllocator<std::pair<const Key, T>>> {
        public:
            using base_type = std::map<Key, T, Compare, mem::PoolTaggedAllocator<std::pair<const Key, T>>>;
            using allocator_type = mem::PoolTaggedAllocator<std::pair<const Key, T>>;

            explicit map(mem::MemTag tag) : base_type(Compare{}, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
                assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            }

            map(const map& other) : base_type(other) {}

            map(map&& other) noexcept : base_type(std::move(other)) {}

            map& operator=(const map& other) {
                base_type::operator=(other);
                return *this;
            }

            map& operator=(map&& other) noexcept {
                base_type::operator=(std::move(other));
                return *this;
            }

            using base_type::base_type;
        };

        // Factory functions for creating tagged containers
        template <class T>
        [[nodiscard]] inline vector<T> make_vector(mem::MemTag tag) {
            auto* mm = mem::MemoryManager::get();
            assert(mm && "MemoryManager not initialized");
            return vector<T>(mem::LinearTaggedAllocator<T>(&mm->arena(tag)));
        }

        template <class K, class V>
        [[nodiscard]] inline unordered_map<K, V> make_unordered_map(mem::MemTag tag) {
            auto& mm = *mem::MemoryManager::get();
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            return unordered_map<K, V>(0, std::conditional_t<std::is_same_v<K, string>, TStringHash, std::hash<K>>{},
                                       std::conditional_t<std::is_same_v<K, string>, TStringEqual, std::equal_to<K>>{},
                                       mem::PoolTaggedAllocator<std::pair<const K, V>>(&mm.arena(tag)));
        }

        template <class Key, class T, class Compare = std::less<Key>>
        [[nodiscard]] inline map<Key, T, Compare> make_map(mem::MemTag tag) {
            auto& mm = *mem::MemoryManager::get();
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
            return map<Key, T, Compare>(Compare{}, mem::PoolTaggedAllocator<std::pair<const Key, T>>(&mm.arena(tag)));
        }

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

    } // namespace stl

} // namespace sf
