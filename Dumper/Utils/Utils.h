#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "TmpUtils.h"

#include "Platform.h"

namespace ASMUtils
{
	/* See IDA or https://c9x.me/x86/html/file_module_x86_id_147.html for reference on the jmp opcode */
	inline bool Is32BitRIPRelativeJump(uintptr_t Address)
	{
		return Address && *reinterpret_cast<uint8_t*>(Address) == 0xE9; /* 48 for jmp, FF for "RIP relative" -- little endian */
	}

	inline uintptr_t Resolve32BitRIPRelativeJumpTarget(uintptr_t Address)
	{
		constexpr int32_t InstructionSizeBytes = 0x5;
		constexpr int32_t InstructionImmediateDisplacementOffset = 0x1;

		const int32_t Offset = *reinterpret_cast<int32_t*>(Address + InstructionImmediateDisplacementOffset);

		/* Add the InstructionSizeBytes because offsets are relative to the next instruction. */
		return Address + InstructionSizeBytes + Offset;
	}

	/* See https://c9x.me/x86/html/file_module_x86_id_147.html */
	inline uintptr_t Resolve32BitRegisterRelativeJump(uintptr_t Address)
	{
		/*
		* 48 FF 25 C1 10 06 00     jmp QWORD [rip+0x610c1]
		*
		* 48 FF 25 <-- Information on the instruction [jump, relative, rip]
		* C1 10 06 00 <-- 32-bit Offset relative to the address coming **after** these instructions (+ 7) [if 48 had hte address 0x0 the offset would be relative to address 0x7]
		*/

		return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
	}

	inline uintptr_t Resolve32BitSectionRelativeCall(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a call, with one less instruction byte */
		return ((Address + 6) + *reinterpret_cast<int32_t*>(Address + 2));
	}

	inline uintptr_t Resolve32BitRelativeCall(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a non-relative call, with two less instruction byte */
		return ((Address + 5) + *reinterpret_cast<int32_t*>(Address + 1));
	}

	inline uintptr_t Resolve32BitRelativeMove(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a relative mov */
		return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
	}

	inline uintptr_t Resolve32BitRelativeLea(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a relative lea */
		return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
	}

	inline uintptr_t Resolve32BitRelativePush(uintptr_t Address)
	{
		/*
		* 68 A0 F8 A6 02		push offset aForwardshading
		*
		* 68 <-- Information on the instruction [push, immediate, 32-bit address]
		* A0 F8 A6 02 <-- 4-byte immediate value representing the address being pushed (little-endian)
		* The address being pushed is the 4-byte value at Address + 1.
		*/
		return *reinterpret_cast<int32_t*>(Address + 1);
	}

	inline uintptr_t Resolve32bitAbsoluteCall(uintptr_t Address)
	{
		/*
		* FF 15 14 93 63 02    call    ds:EnterCriticalSection
		* FF 15 <-- Information on the instruction [call, absolute, 32-bit address]
		* 14 93 63 02 <-- 4-byte absolute address (little-endian)
		*/
		return *reinterpret_cast<int32_t*>(Address + 2);
	}

	inline uintptr_t Resolve32bitAbsoluteMove(uintptr_t Address)
	{
		/*
		* 8B 3D A8 C5 1B 03    mov     edi, dword_31BC5A8
		*
		* 8B 3D <-- Information on the instruction [mov, absolute, 32-bit address to register]
		* A8 C5 1B 03 <-- 4-byte absolute address of the memory operand (little-endian)
		*/
		return *reinterpret_cast<int32_t*>(Address + 2);
	}
}

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
