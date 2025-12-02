#pragma once

#include <cassert>
#include <iostream>
#include <new>
#include <type_traits>
#include <vector>
#include "memory/allocators.h"

namespace sf {

    // Helper functions for memory sizes
    constexpr inline uint64_t gibibytes(uint32_t amount) { return ((amount) * 1024ULL * 1024ULL * 1024ULL); }
    constexpr inline uint64_t mebibytes(uint32_t amount) { return ((amount) * 1024ULL * 1024ULL); }
    constexpr inline uint64_t kibibytes(uint32_t amount) { return ((amount) * 1024ULL); }
    constexpr inline uint64_t gigabytes(uint32_t amount) { return ((amount) * 1000ULL * 1000ULL * 1000ULL); }
    constexpr inline uint64_t megabytes(uint32_t amount) { return ((amount) * 1000ULL * 1000ULL); }
    constexpr inline uint64_t kilobytes(uint32_t amount) { return ((amount) * 1000ULL); }

    namespace mem {

        enum class MemTag : uint8_t {
            Render, // Rendering subsystem
            Logic, // ECS/Game logic
            Physics, // Physics subsystem
            Temp, // Temporary/scratch allocations
            Strings, // String pools
            Filesystem, // File I/O
            Texture, // Texture assets
            Mesh, // Mesh/geometry assets
            Material, // Material assets
            Animation, // Animation assets
            Audio, // Audio assets
            RTTI,
            Application,
            Count
        };

        // compile-time tag tokens (for zero-overhead factories)
        template <MemTag V>
        struct tag_t {
            static constexpr MemTag value = V;
        };

        struct Budgets {
            size_t render = mebibytes(300);
            size_t logic = mebibytes(400);
            size_t physics = mebibytes(400);
            size_t temp = mebibytes(64);
            size_t strings = mebibytes(64);
            size_t fs = mebibytes(256);
            size_t texture = mebibytes(512);
            size_t mesh = mebibytes(256);
            size_t material = mebibytes(128);
            size_t animation = mebibytes(128);
            size_t audio = mebibytes(128);
            size_t rtti = mebibytes(128);
            size_t application = mebibytes(128);
            inline size_t total() const {
                return render + logic + physics + temp + strings + fs + texture + mesh + material + animation + audio + rtti + application;
            }
        };

        class MemoryManager {
        public:
            explicit MemoryManager(Budgets budgets);
            inline ~MemoryManager() noexcept {
                if (g_instance == this)
                    g_instance = nullptr;
            }

            MemoryManager(const MemoryManager&) = delete;
            MemoryManager& operator=(const MemoryManager&) = delete;
            MemoryManager(MemoryManager&&) = delete;
            MemoryManager& operator=(const MemoryManager&&) = delete;

            inline LinearArena& arena(MemTag tag) {
                const auto index = static_cast<int32_t>(tag);
                assert(index >= 0 && index < static_cast<int32_t>(MemTag::Count));
                return m_Arenas[index];
            }

            inline const LinearArena& arena(MemTag tag) const {
                const auto index = static_cast<int32_t>(tag);
                assert(index >= 0 && index < static_cast<int32_t>(MemTag::Count));
                return m_Arenas[index];
            }

            void report(std::ostream& os) const;
            void reset(MemTag tag);
            void reset_all();
            static MemoryManager* get() noexcept { return g_instance; }

        private:
            LinearArena m_Arenas[static_cast<int32_t>(MemTag::Count)];
            std::vector<uint8_t> m_Backing;
            inline static MemoryManager* g_instance = nullptr;
        };

        template <typename T>
        class TempAllocator {
        public:
            using value_type = T;
            using is_always_equal = std::true_type;

            TempAllocator() noexcept : m_Arena(nullptr) {
                // if a global memory manager exists, use its Temp arena
                if (auto* mm = MemoryManager::get()) {
                    m_Arena = &mm->arena(MemTag::Temp);
                }
            }

            explicit TempAllocator(LinearArena* a) noexcept : m_Arena(a) {}

            template <typename U>
            TempAllocator(const TempAllocator<U>& other) noexcept : m_Arena(other.m_Arena) {}

            T* allocate(std::size_t n) {
                assert(m_Arena && "TempAllocator requires a valid arena (MemoryManager not created?)");
                void* p = m_Arena->allocate(n * sizeof(T), alignof(T));
                if (!p)
                    throw std::bad_alloc();
                return static_cast<T*>(p);
            }

            void deallocate(T*, std::size_t) noexcept {
                // no-op; reuse freed on reset
            }

            template <typename U>
            struct rebind {
                using other = TempAllocator<U>;
            };
            template <typename U>
            friend class TempAllocator;

            bool operator==(const TempAllocator& o) const noexcept { return m_Arena == o.m_Arena; }
            bool operator!=(const TempAllocator& o) const noexcept { return m_Arena != o.m_Arena; }

            LinearArena* arena() const noexcept { return m_Arena; }

        private:
            LinearArena* m_Arena;
        };

    } // namespace mem
} // namespace sf

// Placement new/delete operators for tagged allocations
inline void* operator new(size_t size, sf::mem::MemTag tag) {
    auto* mm = sf::mem::MemoryManager::get();
    assert(mm && "MemoryManager not initialized");
    return mm->arena(tag).allocate(size, alignof(std::max_align_t));
}

inline void operator delete(void* ptr, sf::mem::MemTag tag) noexcept {
    // LinearArena doesn't support individual deallocations
    // Memory is reclaimed when arena is reset
    (void)ptr;
    (void)tag;
}

// Macros for convenience
#define mem_new(tag) new (tag)
#define mem_delete(ptr) delete ptr

// Tagged smart pointers
namespace sf::stl {

    // Tagged unique_ptr - custom deleter
    template <typename T>
    class TaggedDeleter {
    public:
        explicit TaggedDeleter(mem::MemTag tag = mem::MemTag::Logic) : m_Tag(tag) {}

        template <typename U>
        TaggedDeleter(const TaggedDeleter<U>& other) noexcept : m_Tag(other.m_Tag) {}

        void operator()(T* ptr) const {
            if (ptr) {
                ptr->~T(); // Call destructor
                // Deallocation happens when arena is reset
            }
        }

        mem::MemTag m_Tag;
    };

} // namespace sf::stl
