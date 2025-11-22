cmake_minimum_required (VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
project(Sapfire VERSION 0.1)
######### SPD LOG ##################
set(no_dev_warnings_backup "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)
set(SPDLOG_BUILD_SHARED ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_EXAMPLE_HO OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_TESTS_HO OFF)
set(SPDLOG_BUILD_BENCH OFF)
# Configure spdlog ASAN to match ENABLE_ASAN option
option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_ASAN)
    set(SPDLOG_SANITIZE_ADDRESS ON)
else()
    set(SPDLOG_SANITIZE_ADDRESS OFF)
endif()
set(SPDLOG_INSTALL OFF)
set(SPDLOG_FMT_EXTERNAL OFF)
set(SPDLOG_FMT_EXTERNAL_HO OFF)
set(SPDLOG_NO_EXCEPTIONS OFF)
set(SPDLOG_BUILD_WARNINGS ON)
set(_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
add_subdirectory(vendor/spdlog)
target_compile_options(spdlog PRIVATE -w)
##################################
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${no_dev_warnings_backup} CACHE INTERNAL "" FORCE)
####### DirectX-Headers ##########
# add_subdirectory(vendor/DirectX-Headers)
##################################
####### DirectXTK12 (Windows only) ##
if(WIN32)
    set(DIRECTX_DXC_PATH vendor/dxc/bin/x64/dxc.exe)
    add_subdirectory(vendor/DirectXTK12)
    ###### D3D12MemoryAllocator ######
    add_subdirectory(vendor/D3D12MemoryAllocator)
endif()
##################################
###### VulkanMemoryAllocator #####
find_package(Vulkan REQUIRED)
add_subdirectory(vendor/VulkanMemoryAllocator)
##################################
###### json ######################
add_subdirectory(vendor/json)
##################################
###### obj ######################
add_subdirectory(vendor/tinyobj)
##################################
add_library(Engine STATIC ${SAPFIRE_TRANSLATION_UNITS})
#target_compile_options(Engine PRIVATE -Wall -Wextra)
target_compile_definitions(Engine PUBLIC SF_BUILD_DLL)

# Optional AddressSanitizer and UndefinedBehaviorSanitizer
if(ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(STATUS "AddressSanitizer and UndefinedBehaviorSanitizer enabled")
    target_compile_options(Engine PUBLIC -fsanitize=address -fsanitize=undefined)
    target_link_options(Engine PUBLIC -fsanitize=address -fsanitize=undefined)
endif()

# Common include directories
target_include_directories(Engine PUBLIC
    include
    vendor/stb
    vendor/json/include
    vendor/tinyobj
    vendor/SDL3/include
)

# Windows-specific (DX12) include directories
if(WIN32)
    target_include_directories(Engine PUBLIC
        vendor/DirectXAgilitySDK/include
        vendor/DirectXAgilitySDK/include/d3dx12
        vendor/DirectXTK12/Inc
        vendor/D3D12MemoryAllocator/include
    )
endif()

# Vulkan include directories
target_include_directories(Engine PUBLIC
    vendor/VulkanMemoryAllocator/include
)
target_include_directories(Engine SYSTEM INTERFACE vendor/spdlog/include)
if(WIN32)
    target_include_directories(Engine SYSTEM INTERFACE vendor/dxc/inc)
endif()
target_precompile_headers(Engine PRIVATE include/engpch.h)

# Common libraries
target_link_libraries(Engine PUBLIC
    spdlog::spdlog
    tinyobjloader
    SDL3::SDL3
    Vulkan::Vulkan
    VulkanMemoryAllocator
    PRIVATE
    nlohmann_json::nlohmann_json
)

# Windows-specific (DX12) libraries
if(WIN32)
    target_link_libraries(Engine PUBLIC
        D3D12.lib
        dxgi.lib
        d3dcompiler.lib
        dxguid.lib
        ${CMAKE_CURRENT_LIST_DIR}/vendor/dxc/lib/x64/dxcompiler.lib
        DirectXTK12
        D3D12MemoryAllocator
    )
endif()
