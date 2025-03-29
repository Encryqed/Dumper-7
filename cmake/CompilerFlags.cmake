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
    
    # Дополнительные флаги для подавления ошибок со static_cast между несвязанными типами
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-microsoft-cast")
    
    # Игнорирование регистра в именах файлов
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nonportable-include-path")
    
    # Отключение строгих проверок для заголовочных файлов
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=header-guard -Wno-error=pragma-once-outside-header")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=invalid-token-paste -Wno-ignored-attributes")
    
    # Отключение предупреждений о Microsoft-специфичных атрибутах
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-microsoft-enum-forward-reference -Wno-microsoft-goto")
    
    # Отключение ошибок сужения значений в перечислениях
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing -Wno-narrowing")
    
    # Разрешение неполных типов и C-style приведений
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-incompatible-pointer-types -Wno-incompatible-function-pointer-types")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cast-function-type -Wno-cast-qual")
    
    # Отключение ошибок с битовыми полями
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-bitfield-constant-conversion")
    
    # Отключение ошибок с FSetBitIterator
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_FSETBITITERATOR_DECREMENT")
    
    # Используем компилятор в режиме максимальной совместимости с MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fdelayed-template-parsing")
    
    # Специальные макросы и определения для UE4 SDK на Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLANG_WORKAROUNDS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTRUCTURE_SIZE_PARANOIA=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
    
    # Макросы для обхода ошибки с InvalidUseOfTDelegate
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEFINE_INVALIDUSEOFTDELEGATE")
    
    # Отключение стандартных проверок Clang для C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override -Wno-overloaded-virtual")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dynamic-class-memaccess -Wno-unused-value")
    
    # Игнорирование ошибок преобразования типов, которые очень часты в UE4 SDK
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=incompatible-pointer-types")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=reinterpret-base-class")
    
    # Игнорирование большинства стандартных ошибок Clang при компиляции UE4 SDK
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
