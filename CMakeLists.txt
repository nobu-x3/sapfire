cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
project(Sandbox VERSION 0.1)

# Initialize submodules if they haven't been initialized
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

add_subdirectory(engine/vendor/SDL3)

include(engine_translation_units.cmake)

add_subdirectory(engine)
# add_subdirectory(sapling)  # Disabled - being refactored
if(BUILD_TESTS)
add_subdirectory(tests)
endif()

add_executable(Sandbox
        sandbox/src/sandbox_app.cpp
        sandbox/src/sandbox_layer.cpp
        sandbox/src/sandbox_game_context.cpp
        )
target_include_directories(Sandbox PUBLIC engine/include PRIVATE sandbox/include)
target_link_libraries(Sandbox
        Engine
        spdlog::spdlog
        )

## Copying dep dlls to sandbox
add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Engine> $<TARGET_FILE_DIR:Sandbox>/$<TARGET_FILE_NAME:Engine>
)
add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:spdlog::spdlog> $<TARGET_FILE_DIR:Sandbox>/$<TARGET_FILE_NAME:spdlog::spdlog>
)

# Windows-specific DLL copying
if(WIN32)
    add_custom_command(TARGET Sandbox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/dxc/_bin/x64/dxcompiler.dll $<TARGET_FILE_DIR:Sandbox>/dxcompiler.dll
    )
    add_custom_command(TARGET Sandbox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/dxc/_bin/x64/dxil.dll $<TARGET_FILE_DIR:Sandbox>/dxil.dll
    )
    add_custom_command(TARGET Sandbox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/DirectXAgilitySDK/_bin/x64/D3D12Core.dll $<TARGET_FILE_DIR:Sandbox>/D3D12Core.dll
    )
    add_custom_command(TARGET Sandbox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/DirectXAgilitySDK/_bin/x64/d3d12SDKLayers.dll $<TARGET_FILE_DIR:Sandbox>/d3d12SDKLayers.dll
    )
    add_custom_command(TARGET Sandbox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/DirectXTK12/_Bin/Desktop_2022_Win10/x64/Release/DirectXTK12.lib $<TARGET_FILE_DIR:Sandbox>/DirectXTK12.lib
    )
endif()
add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Sandbox>/assets
)