#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)

#include "Arch_x86.h"

#define PLATFORM_WINDOWS

#if defined(_WIN64)
#define PLATFORM_WINDOWS64
#else
#define PLATFORM_WINDOWS32
#endif

#elif defined (__ANDROID__)
#error "The dumper does not support android."
#else
#error "Unknown and unsupported platform."
#endif // _WIN32 || _WIN64