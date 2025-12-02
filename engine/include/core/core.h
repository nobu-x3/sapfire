#pragma once

#include <iostream>
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

} // namespace sf

// Include STL types after core types are defined
#include "stl/generational.h"
#include "stl/map.h"
#include "stl/result.h"
#include "stl/shared_ptr.h"
#include "stl/string.h"
#include "stl/types.h"
#include "stl/unique_ptr.h"
#include "stl/unordered_map.h"
#include "stl/vector.h"
