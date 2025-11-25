#pragma once

#include <cassert>
#include <string>
#include <string_view>
#include "memory/memory.h"
#include "core/core.h"

namespace sf::stl {

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

        string(mem::MemTag tag, const std::string& str) : base_type(str.c_str(), allocator_type(&mem::MemoryManager::get()->arena(tag))) {
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

} // namespace sf::stl
