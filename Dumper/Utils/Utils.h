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

struct MemAddress
{
public:
	uintptr_t Address;

private:
	/* Function to determine whether this position is a function-return. Only "ret" instructions with pop operations before them and without immediate values are considered. */
	static bool IsFunctionRet(const uint8_t* Address)
	{
		if (!Address || (Address[0] != 0xC3 && Address[0] != 0xCB))
			return false;

		/* Opcodes representing pop instructions for x64 registers. Pop operations for r8-r15 are prefixed with 0x41. */
		const uint8_t AsmBytePopOpcodes[] = { 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F };

		const uint8_t ByteOneBeforeRet = Address[-1];
		const uint8_t ByteTwoBeforeRet = Address[-2];

		for (const uint8_t AsmPopByte : AsmBytePopOpcodes)
		{
			if (ByteOneBeforeRet == AsmPopByte)
				return true;
		}

		return false;
	}

public:
	inline MemAddress(std::nullptr_t)
		: Address(NULL)
	{
	}
	inline MemAddress(void* Addr)
		: Address(reinterpret_cast<uintptr_t>(Addr))
	{
	}
	inline MemAddress(uintptr_t Addr)
		: Address(Addr)
	{
	}

	explicit operator bool()
	{
		return Address != NULL;
	}

	template<typename T>
	explicit operator T*()
	{
		return reinterpret_cast<T*>(Address);
	}
	operator uintptr_t()
	{
		return Address;
	}

	inline bool operator==(MemAddress Other) const
	{
		return Address == Other.Address;
	}

	inline MemAddress operator+(int Value) const
	{
		return Address + Value;
	}

	inline MemAddress operator-(int Value) const
	{
		return Address - Value;
	}

	template<typename T = void>
	inline T* Get()
	{
		return reinterpret_cast<T*>(Address);
	}

	template<typename T = void>
	inline const T* Get() const
	{
		return reinterpret_cast<const T*>(Address);
	}

	/* 
	* Checks if the current address is a valid 32-bit relative 'jmp' instruction. and returns the address if true. 
	* 
	* If true: Returns resolved jump-target.
	* If false: Returns current address.
	*/
	inline MemAddress ResolveJumpIfInstructionIsJump(MemAddress DefaultReturnValueOnFail = nullptr) const
	{
		if (!ASMUtils::Is32BitRIPRelativeJump(Address))
			return DefaultReturnValueOnFail;

		const uintptr_t TargetAddress = ASMUtils::Resolve32BitRIPRelativeJumpTarget(Address);

		if (!!Platform::IsAddressInProcessRange(TargetAddress))
			return DefaultReturnValueOnFail;

		return TargetAddress;
	}

	/* Helper to find the end of a function based on 'pop' instructions followed by 'ret' */
	inline MemAddress FindFunctionEnd(uint32_t Range = 0xFFFF) const
	{
		if (!Address)
			return nullptr;

		if (Range > 0xFFFF)
			Range = 0xFFFF;

		for (int i = 0; i < Range; i++)
		{
			if (IsFunctionRet(Get<uint8_t>() + i))
				return Address + i;
		}

		return  nullptr;
	}

	/* Helper function to find a Pattern in a Range relative to the current position */
	inline MemAddress RelativePattern(const char* Pattern, int32_t Range, int32_t Relative = 0) const
	{
		if (!Address)
			return nullptr;

		return Platform::FindPatternInRange(Pattern, Get<uint8_t>(), Range, Relative != 0, Relative);
	}

	/*
	* A Function to find calls relative to the instruction pointer (RIP). Other calls are ignored.
	* 
	* Disclaimers:
	*	Negative index to search up, positive index to search down. 
	*	Function considers all E8 bytes as 'call' instructsion, that would make for a valid call (to address within process-bounds).
	* 
	* OneBasedFuncIndex -> Index of a function we want to find, n-th sub_ in IDA starting from this MemAddress
	* IsWantedTarget -> Allows for the caller to pass a callback to verify, that the function at index n is the target we're looking for; else continue searching for a valid target.
	*/
	inline MemAddress GetRipRelativeCalledFunction(int32_t OneBasedFuncIndex, bool(*IsWantedTarget)(MemAddress CalledAddr) = nullptr) const
	{
		if (!Address || OneBasedFuncIndex == 0)
			return nullptr;

		const int32_t Multiply = OneBasedFuncIndex > 0 ? 1 : -1;

		/* Returns Index if FunctionIndex is positive, else -1 if the index is less than 0 */
		auto GetIndex = [=](int32_t Index) -> int32_t { return Index * Multiply; };

		constexpr int32_t RealtiveCallOpcodeCount = 0x5;

		int32_t NumCalls = 0;

		for (int i = 0; i < 0xFFF; i++)
		{
			const int32_t Index = GetIndex(i);

			/* If this isn't a call, we don't care about it and want to continue */
			if (Get<uint8_t>()[Index] != 0xE8)
				continue;

			const int32_t RelativeOffset = *reinterpret_cast<int32_t*>(Address + Index + 0x1 /* 0xE8 byte */);
			MemAddress RelativeCallTarget = Address + Index + RelativeOffset + RealtiveCallOpcodeCount;

			if (!Platform::IsAddressInProcessRange(RelativeCallTarget))
				continue;

			if (++NumCalls == abs(OneBasedFuncIndex))
			{
				/* This is not the target we wanted, even tho it's at the right index. Decrement the index to the value before and check if the next call satisfies the custom-condition. */
				if (IsWantedTarget && !IsWantedTarget(RelativeCallTarget))
				{
					--NumCalls;
					continue;
				}

				return RelativeCallTarget;
			}
		}

		return nullptr;
	}

	/* Note: Unrealiable */
	inline MemAddress FindNextFunctionStart() const
	{
		if (!Address)
			return MemAddress(nullptr);

		uintptr_t FuncEnd = (uintptr_t)FindFunctionEnd();

		return FuncEnd % 0x10 != 0 ? FuncEnd + (0x10 - (FuncEnd % 0x10)) : FuncEnd;
	}
};

template<typename Type = const char*>
inline MemAddress FindUnrealExecFunctionByString(Type RefStr, void* StartAddress = nullptr)
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

		MemAddress AsAddress(Address);

		/* A signature specifically made for UFunctions-construction functions. If this signature is found we're in a function that we *don't* want. */
		if (AsAddress.RelativePattern("48 8B 05 ? ? ? ? 48 85 C0 75 ? 48 8D 15", 0x28) != nullptr)
			return false;

		return true;
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

				return { PossibleExecFuncAddress };
			}
		}
		else
		{
			if (wcsncmp(reinterpret_cast<const wchar_t*>(RefStr), reinterpret_cast<const wchar_t*>(PossibleStringAddress), RefStrLen) == 0 && IsValidExecFunctionNotSetupFunc(PossibleExecFuncAddress))
			{
				// std::wcerr << L"FoundStr wref: " << reinterpret_cast<const wchar_t*>(PossibleStringAddress) << L"\n";

				return { PossibleExecFuncAddress };
			}
		}
	}

	return nullptr;
}
