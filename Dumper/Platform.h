#pragma once

#ifdef _WIN32 || _WIN64

#include "PlatformWindows.h"

namespace Platform = PlatformWindows;

#elif defined (__ANDROID__)
#error "The dumper does not support android."
#elif defined (__linux__)
#error "The dumper does not support linux."
#else
#error "Unknown and unsupported platform."
#endif // _WIN32 || _WIN64

