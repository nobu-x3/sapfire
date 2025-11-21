#pragma once

// Platform detection
#ifdef _WIN32
	#ifdef _WIN64
		#ifndef SF_PLATFORM_WINDOWS
			#define SF_PLATFORM_WINDOWS
		#endif
	#endif
#elif defined(__linux__)
	#ifndef SF_PLATFORM_LINUX
		#define SF_PLATFORM_LINUX
	#endif
#elif defined(__APPLE__)
	#include <TargetConditionals.h>
	#if TARGET_OS_MAC
		#ifndef SF_PLATFORM_MACOS
			#define SF_PLATFORM_MACOS
		#endif
	#endif
#else
	#error "Unknown platform!"
#endif

// Render API selection
#if defined(SF_PLATFORM_WINDOWS)
	#ifndef SF_RENDER_API_DX12
		#define SF_RENDER_API_DX12
	#endif
#elif defined(SF_PLATFORM_LINUX) || defined(SF_PLATFORM_MACOS)
	#ifndef SF_RENDER_API_VULKAN
		#define SF_RENDER_API_VULKAN
	#endif
#endif
