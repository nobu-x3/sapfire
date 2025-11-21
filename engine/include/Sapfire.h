#pragma once
////////// CORE //////////////////
#include "core/application.h"
#include "core/core.h"
#include "core/game_context.h"
#include "core/layer.h"
#include "core/layer_stack.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/rtti.h"
#include "core/uuid.h"
#include "core/file_system.h"
/////////////////////////////////

///////// STL ///////////////////
#include "core/stl/unique_ptr.h"
#include "core/stl/shared_ptr.h"
/////////////////////////////////

///////// COMPONENTS ////////////
#include "components/component.h"
#include "components/ec_manager.h"
#include "components/entity.h"
#include "components/name_component.h"
#include "components/transform.h"
#include "components/render_component.h"
#include "components/test_custom_component.h"
/////////////////////////////////

//////// ASSETS /////////////////
#include "assets/mesh_manager.h"
#include "assets/texture_manager.h"
#include "assets/material_manager.h"
#include "assets/scene_writer.h"
/////////////////////////////////

//////// EVENTS /////////////////
#include "events/event.h"
/////////////////////////////////

/////// TOOLS ///////////////////
#include "tools/profiling.h"
/////////////////////////////////
/////// RENDERING ///////////////
#include "render/camera.h"
#include "render/render_compat.h"
#include "render/lights.h"
#include "render/material.h"
#include "render/upload_buffer.h"
#include "render/window.h"
/////////////////////////////////
