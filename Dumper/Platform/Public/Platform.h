#pragma once

#ifdef _WIN32 || _WIN64

#include "Platform/Private/PlatformWindows.h"

namespace Platform = PlatformWindows;

#define PLATFORM_WINDOWS

#if defined(_WIN64)
#define PLATFORM_WINDOWS64
#else
#define PLATFORM_WINDOWS32
#endif

#elif defined (__ANDROID__)
#error "The dumper does not support android."
#elif defined (__linux__)
#error "The dumper does not support linux."
#else
#error "Unknown and unsupported platform."
#endif // _WIN32 || _WIN64

