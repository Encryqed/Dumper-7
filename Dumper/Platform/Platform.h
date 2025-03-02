#pragma once

enum class EPlatform
{
	Windows,

	Invalid = 0xFF
};

#ifdef _WIN32
#include "Windows/Memory.h"
#else
#error "Unsupported platform!"
#endif


namespace Platform
{
#ifdef _WIN32
	// Bring all Windows implementations of platform-specific functions into scope.
	namespace CurrentPlatform = PlatformWindows;

	constexpr EPlatform Current = EPlatform::Windows;
#else
#error "Unsupported platform!"

	constexpr EPlatform Current = EPlatform::Invalid;
#endif

	inline std::pair<uintptr_t, uintptr_t> GetModuleBaseAndSize(const char* const ModuleName = nullptr)
	{
		return CurrentPlatform::GetModuleBaseAndSize(ModuleName);
	}

	inline uintptr_t GetOffset(const uintptr_t Address)
	{
		return CurrentPlatform::GetOffset(Address);
	}

	inline uintptr_t GetOffset(const void* Address)
	{
		return CurrentPlatform::GetOffset(Address);
	}

	inline bool IsAddrInAnyModule(const void* Address)
	{
		return CurrentPlatform::IsAddrInAnyModule(Address);
	}

	inline bool IsAddrInAnyModule(const uintptr_t Address)
	{
		return CurrentPlatform::IsAddrInAnyModule(Address);
	}

	inline bool IsInProcessRange(const uintptr_t Address)
	{
		return CurrentPlatform::IsInProcessRange(Address);
	}

	inline bool IsInProcessRange(const void* Address)
	{
		return CurrentPlatform::IsInProcessRange(Address);
	}

	inline std::pair<uintptr_t, DWORD> GetSectionByName(const uintptr_t ImageBase, const std::string& ReqestedSectionName)
	{
		return CurrentPlatform::GetSectionByName(ImageBase, ReqestedSectionName);
	}

	inline void* GetImportAddress(const uintptr_t ModuleBase, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		return CurrentPlatform::GetImportAddress(ModuleBase, ModuleToImportFrom, SearchFunctionName);
	}

	inline void* GetImportAddress(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		return CurrentPlatform::GetImportAddress(SearchModuleName, ModuleToImportFrom, SearchFunctionName);
	}
}

