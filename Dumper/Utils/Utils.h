#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "TmpUtils.h"

#include "Platform.h"


inline std::pair<uintptr_t, uintptr_t> GetImageBaseAndSize(const char* const ModuleName = nullptr)
{
	const uintptr_t ImageBase = Platform::GetModuleBase(ModuleName);
	const PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew);

	return { ImageBase, NtHeader->OptionalHeader.SizeOfImage };
}


template<typename Type = const char*>
inline void* FindUnrealExecFunctionByString(Type RefStr, void* StartAddress = nullptr)
{
	const auto [ImageBase, ImageSize] = GetImageBaseAndSize();

	uint8_t* SearchStart = StartAddress ? reinterpret_cast<uint8_t*>(StartAddress) : reinterpret_cast<uint8_t*>(ImageBase);
	DWORD SearchRange = ImageSize;

	const int32_t RefStrLen = StrlenHelper(RefStr);

	static auto IsValidExecFunctionNotSetupFunc = [](uintptr_t Address) -> bool
	{
		/* 
		* UFuntion construction functions setting up exec functions always start with these asm instructions:
		* sub rsp, 28h
		* 
		* In opcode bytes: 48 83 EC 28
		*/
		if (*reinterpret_cast<int32_t*>(Address) == 0x284883EC || *reinterpret_cast<int32_t*>(Address) == 0x4883EC28)
			return false;

		const void* SigOccurence = Platform::FindPatternInRange("48 8B 05 ? ? ? ? 48 85 C0 75 ? 48 8D 15", Address, 0x28);
		
		/* A signature specifically made for UFunctions-construction functions. If this signature is found we're in a function that we *don't* want. */
		return SigOccurence == nullptr;
	};

	for (uintptr_t i = 0; i < (SearchRange - 0x8); i += sizeof(void*))
	{
		const uintptr_t PossibleStringAddress = *reinterpret_cast<uintptr_t*>(SearchStart + i);
		const uintptr_t PossibleExecFuncAddress = *reinterpret_cast<uintptr_t*>(SearchStart + i + sizeof(void*));

		if (PossibleStringAddress == PossibleExecFuncAddress)
			continue;

		if (!Platform::IsAddressInProcessRange(PossibleStringAddress) || !Platform::IsAddressInProcessRange(PossibleExecFuncAddress))
			continue;

		if constexpr (std::is_same<Type, const char*>())
		{
			if (strncmp(reinterpret_cast<const char*>(RefStr), reinterpret_cast<const char*>(PossibleStringAddress), RefStrLen) == 0 && IsValidExecFunctionNotSetupFunc(PossibleExecFuncAddress))
			{
				// std::cerr << "FoundStr ref: " << reinterpret_cast<const char*>(PossibleStringAddress) << "\n";

				return reinterpret_cast<void*>(PossibleExecFuncAddress);
			}
		}
		else
		{
			if (wcsncmp(reinterpret_cast<const wchar_t*>(RefStr), reinterpret_cast<const wchar_t*>(PossibleStringAddress), RefStrLen) == 0 && IsValidExecFunctionNotSetupFunc(PossibleExecFuncAddress))
			{
				// std::wcerr << L"FoundStr wref: " << reinterpret_cast<const wchar_t*>(PossibleStringAddress) << L"\n";

				return reinterpret_cast<void*>(PossibleExecFuncAddress);
			}
		}
	}

	return nullptr;
}
