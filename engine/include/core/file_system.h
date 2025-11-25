#pragma once

#include <filesystem>
#include "core.h"

namespace sf::fs {

    class FileSystem {
    public:
        static void locate_root_directory();

        static const char* root_directory() { return s_RootDirectoryPath.c_str(); }

    private:
        static inline std::string s_RootDirectoryPath{};
    };

    inline bool exists(const stl::string_view asset_path) { return std::filesystem::exists(asset_path); }

    inline stl::string extension(const stl::string_view asset_path) {
        std::filesystem::path path(asset_path);
        return stl::string(mem::MemTag::Filesystem, path.extension().string());
    }

    inline stl::string relative_path(const stl::string_view asset_path) {
        std::filesystem::path path = std::filesystem::relative(asset_path);
        if (path.is_relative()) {
            return stl::string(mem::MemTag::Filesystem, path.relative_path().string());
        }
        std::filesystem::path corrected_path =
            std::filesystem::relative(std::filesystem::path(asset_path), std::filesystem::absolute(FileSystem::root_directory()));
        return stl::string(mem::MemTag::Filesystem, corrected_path.relative_path().string());
    }

    inline stl::string full_path(const stl::string_view asset_path) {
        std::filesystem::path path = std::filesystem::absolute(asset_path);
        if (path.is_absolute()) {
            return stl::string(mem::MemTag::Filesystem, std::filesystem::absolute(asset_path).string());
        }
        std::filesystem::path corrected_path{std::filesystem::current_path().string() + "/" + std::string{asset_path}};
        return stl::string(mem::MemTag::Filesystem, std::filesystem::absolute(corrected_path).string());
    }

    inline std::wstring full_path(const std::wstring_view asset_path) {
        std::filesystem::path path = std::filesystem::absolute(asset_path);
        if (path.is_absolute()) {
            return std::filesystem::absolute(asset_path).wstring();
        }
        std::filesystem::path corrected_path{std::filesystem::current_path().wstring() + L"/" + std::wstring{asset_path}};
        return std::filesystem::absolute(corrected_path).wstring();
    }

    inline stl::string file_name(const stl::string_view asset_path) {
        std::filesystem::path path(asset_path);
        return stl::string(mem::MemTag::Filesystem, path.filename().string());
    }

} // namespace sf::fs
