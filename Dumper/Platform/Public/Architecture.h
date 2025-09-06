#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)

#include "Platform/Private/Arch_x86.h"

#elif defined (__ANDROID__)
#error "The dumper does not support android."
#else
#error "Unknown and unsupported platform."
#endif // _WIN32 || _WIN64