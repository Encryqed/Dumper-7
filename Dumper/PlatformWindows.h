#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

/*
Interface:
	ASMUtils:
		- bool Is32BitRIPRelativeJump(uintptr_t Address)
		-
		- uintptr_t Resolve32BitRIPRelativeJumpTarget(uintptr_t Address)
		- uintptr_t Resolve32BitRegisterRelativeJump(uintptr_t Address)
		- uintptr_t Resolve32BitSectionRelativeCall(uintptr_t Address)
		- uintptr_t Resolve32BitRelativeCall(uintptr_t Address)
		- uintptr_t Resolve32BitRelativeMove(uintptr_t Address)
		- uintptr_t Resolve32BitRelativeLea(uintptr_t Address)
		- uintptr_t Resolve32BitRelativePush(uintptr_t Address)
		- uintptr_t Resolve32bitAbsoluteCall(uintptr_t Address)
		- uintptr_t Resolve32bitAbsoluteMove(uintptr_t Address)
		-
		-
	WindowsPE:
		-
		-
	General Interface:
		- uintptr_t GetModuleBase(const char* const ModuleName = nullptr)
		- uintptr_t GetOffset(uintptr_t Address, const char* const ModuleName = nullptr)
		- uintptr_t GetOffset(void* Address, const char* const ModuleName = nullptr)
		-
		- SectionInfo GetSectionInfo(const std::string& SectionName)
		- void IterateSectionWithCallback(const SectionInfo& Info, std::function<bool(void* Address)> Callback, uint32_t Granularity = 0x4, uint32_t OffsetFromEnd = 0x0);
		-
		- bool IsAddressInAnyModule(const uintptr_t Address)
		- bool IsAddressInAnyModule(const void* Address)
		- bool IsAddressInProcessRange(const uintptr_t Address)
		- bool IsAddressInProcessRange(const void* Address)
		- bool IsBadReadPtr(const uintptr_t Address)
		- bool IsBadReadPtr(const void* Address)
		-
		- void* GetAddressOfImportedFunction(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
		- void* GetAddressOfImportedFunctionFromAnyModule(const char* ModuleToImportFrom, const char* SearchFunctionName)
		-
		- std::pair<const void*, int32_t> IterateVTableFunctions(void** VTable, const std::function<bool(const uint8_t* Addr, int32_t Index)>& CallBackForEachFunc, int32_t NumFunctions = 0x150, int32_t OffsetFromStart = 0x0)
		-
*/

// Purposefully opaque section-handle, must be obtained from function-call
struct SectionInfo
{
private:
	uint8_t Data[0x10] = { 0x0 };

public:
	// A section info must be obtained from a platform-function
	SectionInfo() = delete;

public:
	inline bool IsValid() const
	{
		for (int i = 0; i < sizeof(Data); i++)
		{
			// An invalid SectionInfo object is all 0
			if (Data[i] != 0x0)
				return true;
		}

		return false;
	}
};

namespace PlatformWindows
{
	uintptr_t GetModuleBase(const char* const ModuleName = nullptr);
	uintptr_t GetOffset(const uintptr_t Address, const char* const ModuleName = nullptr);
	uintptr_t GetOffset(const void* Address, const char* const ModuleName = nullptr);
	
	SectionInfo GetSectionInfo(const std::string& SectionName, const char* const ModuleName = nullptr);
	void* IterateSectionWithCallback(const SectionInfo& Info, const std::function<bool(void* Address)>& Callback, uint32_t Granularity = 0x4, uint32_t OffsetFromEnd = 0x0);
	void* IterateAllSectionsWithCallback(const std::function<bool(void* Address)>& Callback, uint32_t Granularity = 0x4, uint32_t OffsetFromEnd = 0x0, const char* const ModuleName = nullptr);

	bool IsAddressInAnyModule(const uintptr_t Address);
	bool IsAddressInAnyModule(const void* Address);
	bool IsAddressInProcessRange(const uintptr_t Address);
	bool IsAddressInProcessRange(const void* Address);
	bool IsBadReadPtr(const uintptr_t Address);
	bool IsBadReadPtr(const void* Address);

	const void* GetAddressOfImportedFunction(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName);
	const void* GetAddressOfImportedFunctionFromAnyModule(const char* ModuleToImportFrom, const char* SearchFunctionName);

	template<bool bShouldResolve32BitJumps = true>
	std::pair<const void*, int32_t> IterateVTableFunctions(void** VTable, const std::function<bool(const uint8_t* Address, int32_t Index)>&CallBackForEachFunc, int32_t NumFunctions = 0x150, int32_t OffsetFromStart = 0x0);

	void* FindPattern(const char* Signature, const uint32_t Offset = 0, const bool bSearchAllSections = false, const uintptr_t StartAddress = 0x0, const char* const ModuleName = nullptr);
	void* FindPatternInRange(const char* Signature, const uint8_t* Start, const uintptr_t Range, const bool bRelative = false, const uint32_t Offset = 0);
	void* FindPatternInRange(std::vector<int>&& Signature, const uint8_t* Start, const uintptr_t Range, const bool bRelative = false, uint32_t Offset = 0, const uint32_t SkipCount = 0);


	/* Slower than FindByString */
	template<bool bCheckIfLeaIsStrPtr = false, typename CharType = char>
	inline void* FindByStringInAllSections(const CharType* RefStr, const bool bSearchOnlyExecutableSections = true, const uintptr_t StartAddress = 0x0, int32_t Range = 0x0, const char* const ModuleName = nullptr);


	template<bool bCheckIfLeaIsStrPtr, typename CharType>
	inline void* FindStringInRange(const CharType* RefStr, const uintptr_t StartAddress, const int32_t Range);
}

