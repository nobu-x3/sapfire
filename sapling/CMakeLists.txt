cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
project(Sapling VERSION 0.1)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Collect source files
set(SAPLING_SOURCES
    src/main.cpp
    src/editor_context.cpp
    src/main_window.cpp
    src/project_launcher_dialog.cpp
    src/splash_screen.cpp
    src/project_manager.cpp
    src/widgets/rtti_drawer.cpp
    src/widgets/scene_view_widget.cpp
    src/widgets/scene_hierarchy_widget.cpp
    src/widgets/entity_inspector_widget.cpp
    src/widgets/asset_browser_widget.cpp
    src/widgets/file_system_model.cpp
    src/widgets/asset_delegate.cpp
    src/widgets/asset_filter_proxy.cpp
)

set(SAPLING_HEADERS
    include/editor_context.h
    include/main_window.h
    include/editor_utils.h
    include/project_launcher_dialog.h
    include/splash_screen.h
    include/project_manager.h
    include/widgets/rtti_drawer.h
    include/widgets/scene_view_widget.h
    include/widgets/scene_hierarchy_widget.h
    include/widgets/entity_inspector_widget.h
    include/widgets/asset_browser_widget.h
    include/widgets/file_system_model.h
    include/widgets/asset_delegate.h
    include/widgets/asset_filter_proxy.h
)


set(SAPLING_RESOURCES
    resources/editor_icons.qrc
)

# Create executable
add_executable(Sapling
    ${SAPLING_SOURCES}
    ${SAPLING_HEADERS}
    ${SAPLING_RESOURCES}
)

# Include directories
target_include_directories(Sapling PUBLIC
    ${ROOT_PATH_ABS}/engine/include
    include
)

# Link libraries
target_link_libraries(Sapling
    PRIVATE
    Engine
    Qt6::Widgets
    SDL3::SDL3
)

# Platform-specific DLL copying (Windows only)
if(WIN32)
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Engine> $<TARGET_FILE_DIR:Sapling>/$<TARGET_FILE_NAME:Engine>
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:spdlog::spdlog> $<TARGET_FILE_DIR:Sapling>/$<TARGET_FILE_NAME:spdlog::spdlog>
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/engine/vendor/dxc/_bin/x64/dxcompiler.dll $<TARGET_FILE_DIR:Sapling>/dxcompiler.dll
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/engine/vendor/dxc/_bin/x64/dxil.dll $<TARGET_FILE_DIR:Sapling>/dxil.dll
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/engine/vendor/DirectXAgilitySDK/_bin/x64/D3D12Core.dll $<TARGET_FILE_DIR:Sapling>/D3D12Core.dll
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/engine/vendor/DirectXAgilitySDK/_bin/x64/d3d12SDKLayers.dll $<TARGET_FILE_DIR:Sapling>/d3d12SDKLayers.dll
    )
    add_custom_command(TARGET Sapling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/engine/vendor/DirectXTK12/_Bin/Desktop_2022_Win10/x64/Release/DirectXTK12.lib $<TARGET_FILE_DIR:Sapling>/DirectXTK12.lib
    )
endif()

# Copy assets (cross-platform)
add_custom_command(TARGET Sapling POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Sapling>/assets
)
add_custom_command(TARGET Sapling POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/sapling/editor_assets $<TARGET_FILE_DIR:Sapling>/editor_assets
)
