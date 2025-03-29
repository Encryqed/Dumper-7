set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Set compiler options for different compilers and build types

# Common options for all compilers
set(COMMON_CXX_FLAGS_DEBUG "")
set(COMMON_CXX_FLAGS_RELEASE "")

# Apply common flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_CXX_FLAGS_RELEASE}")

# MSVC specific options
if(MSVC)
    # Enable multiprocessor compilation for MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /std:c++20")
    message(STATUS "Enabled multiprocessor compilation for MSVC")

    # Disable warnings for release build only
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W0")
    
    # Disable specific MSVC warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819 /wd4251")
    
    # Enable intrinsics for XORSTR
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")

# Clang specific options
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Set modern C++20 standard
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    message(STATUS "Using Clang with C++20 standard")
    
    # Add warnings and optimizations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    
    # Optimizations for release build
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    
    # Debug info for debug build
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    
    # Disable specific warnings that might be too noisy
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces -Wno-inconsistent-missing-override")
    
    # Enable colored diagnostics
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    
    # Enable intrinsics for xorstr (AVX/SSE support)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -msse4.2")
    
    # Silence compile erros Unreal Engine SDK
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-conversion -Wno-microsoft-template-shadow -Wno-pragma-once-outside-header")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-mismatched-tags -Wno-invalid-constexpr -Wno-unused-private-field")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-bool-conversion")
    
    # Allow reinterpret_cast in constexpr expressions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions")
    
    # Support for Microsoft ABI features
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-compatibility -fms-compatibility-version=19.29")
    
    # Enable invalid type conversions for compatibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
    
    # Replace -fasm-blocks to more compatibility option
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fasm")
    
    # Additional flags to suppress errors with static_cast between unrelated types
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-microsoft-cast")
    
    # Ignore case in file names
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nonportable-include-path")
    
    # Disable strict checks for header files
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=header-guard -Wno-error=pragma-once-outside-header")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=invalid-token-paste -Wno-ignored-attributes")
    
    # Disable warnings about Microsoft-specific attributes
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-microsoft-enum-forward-reference -Wno-microsoft-goto")
    
    # Disable errors about narrowing conversions in enums
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing -Wno-narrowing")
    
    # Allow incomplete types and C-style casts
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-incompatible-pointer-types -Wno-incompatible-function-pointer-types")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cast-function-type -Wno-cast-qual")
    
    # Disable errors with bit fields
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-bitfield-constant-conversion")
    
    # Disable errors with FSetBitIterator
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_FSETBITITERATOR_DECREMENT")
    
    # Use the compiler in maximum compatibility mode with MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fdelayed-template-parsing")
    
    # Special macros and definitions for UE4 SDK on Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLANG_WORKAROUNDS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTRUCTURE_SIZE_PARANOIA=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
    
    # Macros to workaround InvalidUseOfTDelegate errors
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEFINE_INVALIDUSEOFTDELEGATE")
    
    # Disable standard Clang checks for C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override -Wno-overloaded-virtual")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dynamic-class-memaccess -Wno-unused-value")
    
    # Ignore type conversion errors that are very frequent in UE4 SDK
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=incompatible-pointer-types")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=reinterpret-base-class")
    
    # Ignore most standard Clang errors when compiling UE4 SDK
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

# GCC specific options
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Set modern C++20 standard
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    message(STATUS "Using GCC with C++20 standard")
    
    # Add warnings and optimizations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    
    # Optimizations for release build
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    
    # Debug info for debug build
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    
    # Enable intrinsics for xorstr
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -msse4.2")
endif()

# Print compiler flags
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags (Debug): ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "C++ flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
