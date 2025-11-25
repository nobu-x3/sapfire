#pragma once

#include <array>
#include <bitset>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <sstream>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include "memory/memory.h"

namespace sf::stl {

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
    using wstring_view = std::wstring_view;

    using mutex = std::mutex;
    using recursive_mutex = std::recursive_mutex;

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

    template <size_t Size>
    using bitset = std::bitset<Size>;

    // Temporary string using TempAllocator
    using tmp_string = std::basic_string<char, std::char_traits<char>, mem::TempAllocator<char>>;

    // Tagged stringstream
    using tstringstream = std::basic_stringstream<char, std::char_traits<char>, mem::LinearTaggedAllocator<char>>;

} // namespace sf::stl
