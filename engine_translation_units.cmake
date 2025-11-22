set(RELATIVE_SAPFIRE_TRANSLATION_UNITS
        engine/include/core/entry.h
        engine/src/core/logger.cpp
        engine/src/core/application.cpp
        engine/src/core/layer_stack.cpp
        engine/src/core/input.cpp
        engine/src/core/file_system.cpp
        engine/src/core/game_context.cpp
        engine/src/core/uuid.cpp
        engine/src/core/rtti.cpp
        engine/src/core/memory.cpp
        engine/src/math/quat.cpp
        engine/src/math/mat4.cpp
        engine/src/assets/mesh_manager.cpp
        engine/src/assets/texture_manager.cpp
        engine/src/assets/material_manager.cpp
        engine/src/assets/asset_manager.cpp
        engine/src/assets/scene_writer.cpp
        engine/src/components/components.cpp
        engine/src/components/transform.cpp
        engine/src/components/name_component.cpp
        engine/src/components/entity.cpp
        engine/src/components/ec_manager.cpp
        engine/src/components/test_custom_component.cpp
        engine/src/components/movement_component.cpp
        engine/src/components/render_component.cpp
        engine/src/components/anim_component.cpp
        engine/src/tools/texture_loader.cpp
        engine/src/tools/obj_loader.cpp
        engine/src/render/camera.cpp
        engine/src/render/d3d_primitives.cpp
        engine/src/render/frame_data.cpp
        engine/src/render/render_backend.cpp
        engine/src/render/resource_types.cpp
        engine/src/physics/physics_engine.cpp
        engine/src/render/window.cpp
        engine/src/animation/skinned_data.cpp
        engine/src/animation/anim_manager.cpp
)

if(WIN32)
set(RELATIVE_SAPFIRE_TRANSLATION_UNITS ${RELATIVE_SAPFIRE_TRANSLATION_UNITS}
        engine/src/render/dx12/dx12_command_queue.cpp
        engine/src/render/dx12/dx12_context.cpp
        engine/src/render/dx12/dx12_descriptor_heap.cpp
        engine/src/render/dx12/dx12_graphics_device.cpp
        engine/src/render/dx12/dx12_memory_allocator.cpp
        engine/src/render/dx12/dx12_pipeline_state.cpp
        engine/src/render/dx12/dx12_shader_compiler.cpp
)
endif()

# Vulkan backend (cross-platform)
set(RELATIVE_SAPFIRE_TRANSLATION_UNITS ${RELATIVE_SAPFIRE_TRANSLATION_UNITS}
        engine/src/render/vulkan/vk_command_queue.cpp
        engine/src/render/vulkan/vk_context.cpp
        engine/src/render/vulkan/vk_descriptor_heap.cpp
        engine/src/render/vulkan/vk_graphics_device.cpp
        engine/src/render/vulkan/vk_memory_allocator.cpp
        engine/src/render/vulkan/vk_pipeline_state.cpp
        engine/src/render/vulkan/vk_shader_compiler.cpp
        engine/src/render/vulkan/vk_type_conversions.cpp
)

foreach(TU ${RELATIVE_SAPFIRE_TRANSLATION_UNITS})
    set(TU ${CMAKE_CURRENT_SOURCE_DIR}/${TU})
    list(APPEND SAPFIRE_TRANSLATION_UNITS ${TU})
endforeach(TU)
