#pragma once

#include "Arch_x86.h"
#include "Platform.h"

// The processor (x86-64) only translates 52bits (or 57 bits) of a virtual address into a physical address and the unused bits need to be all 0 or all 1.
bool Architecture_x86_64::IsValid64BitVirtualAddress(const uintptr_t Address)
{
	constexpr uint64_t BitMask = 0b1111'1111ull << 56;

	return (Address & BitMask) == BitMask || (Address & BitMask) == 0x0;
}

bool Architecture_x86_64::IsValid64BitVirtualAddress(const void* Address)
{
	return IsValid64BitVirtualAddress(reinterpret_cast<const uintptr_t>(Address));
}


/* See IDA or https://c9x.me/x86/html/file_module_x86_id_147.html for reference on the jmp opcode */
bool Architecture_x86_64::Is32BitRIPRelativeJump(const uintptr_t Address)
{
	return Address && *reinterpret_cast<uint8_t*>(Address) == 0xE9; /* 48 for jmp, FF for "RIP relative" -- little endian */
}

uintptr_t Architecture_x86_64::Resolve32BitRIPRelativeJumpTarget(const uintptr_t Address)
{
	constexpr int32_t InstructionSizeBytes = 0x5;
	constexpr int32_t InstructionImmediateDisplacementOffset = 0x1;

	const int32_t Offset = *reinterpret_cast<int32_t*>(Address + InstructionImmediateDisplacementOffset);

	/* Add the InstructionSizeBytes because offsets are relative to the next instruction. */
	return Address + InstructionSizeBytes + Offset;
}

uintptr_t Architecture_x86_64::Resolve32BitRegisterRelativeJump(const uintptr_t Address)
{
	/*
	* 48 FF 25 C1 10 06 00     jmp QWORD [rip+0x610c1]
	*
	* 48 FF 25 <-- Information on the instruction [jump, relative, rip]
	* C1 10 06 00 <-- 32-bit Offset relative to the address coming **after** these instructions (+ 7) [if 48 had hte address 0x0 the offset would be relative to address 0x7]
	*/

	return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
}

uintptr_t Architecture_x86_64::Resolve32BitSectionRelativeCall(const uintptr_t Address)
{
	/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a call, with one less instruction byte */
	return ((Address + 6) + *reinterpret_cast<int32_t*>(Address + 2));
}

uintptr_t Architecture_x86_64::Resolve32BitRelativeCall(const uintptr_t Address)
{
	/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a non-relative call, with two less instruction byte */
	return ((Address + 5) + *reinterpret_cast<int32_t*>(Address + 1));
}

uintptr_t Architecture_x86_64::Resolve32BitRelativeMove(const uintptr_t Address)
{
	/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a relative mov */
	return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
}

uintptr_t Architecture_x86_64::Resolve32BitRelativeLea(const uintptr_t Address)
{
	/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a relative lea */
	return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
}

uintptr_t Architecture_x86_64::Resolve32BitRelativePush(const uintptr_t Address)
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

// 32-bit only
uintptr_t Architecture_x86_64::Resolve32bitAbsoluteCall(const uintptr_t Address)
{
	/*
	* FF 15 14 93 63 02    call    ds:EnterCriticalSection
	* FF 15 <-- Information on the instruction [call, absolute, 32-bit address]
	* 14 93 63 02 <-- 4-byte absolute address (little-endian)
	*/
	return *reinterpret_cast<int32_t*>(Address + 2);
}

// 32-bit only
uintptr_t Architecture_x86_64::Resolve32bitAbsoluteMove(const uintptr_t Address)
{
	/*
	* 8B 3D A8 C5 1B 03    mov     edi, dword_31BC5A8
	*
	* 8B 3D <-- Information on the instruction [mov, absolute, 32-bit address to register]
	* A8 C5 1B 03 <-- 4-byte absolute address of the memory operand (little-endian)
	*/
	return *reinterpret_cast<int32_t*>(Address + 2);
}


bool Architecture_x86_64::IsFunctionRet(const uintptr_t Address)
{
	const uint8_t* AsBytePtr = reinterpret_cast<const uint8_t*>(Address);

	if (!AsBytePtr || (AsBytePtr[0] != 0xC3 && AsBytePtr[0] != 0xCB))
		return false;

	/* Opcodes representing pop instructions for x64 registers. Pop operations for r8-r15 are prefixed with 0x41. */
	const uint8_t AsmBytePopOpcodes[] = { 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F };

	const uint8_t ByteOneBeforeRet = AsBytePtr[-1];
	const uint8_t ByteTwoBeforeRet = AsBytePtr[-2];

	for (const uint8_t AsmPopByte : AsmBytePopOpcodes)
	{
		if (ByteOneBeforeRet == AsmPopByte)
			return true;
	}

	return false;
}

uintptr_t Architecture_x86_64::ResolveJumpIfInstructionIsJump(const uintptr_t Address, const uintptr_t DefaultReturnValueOnFail)
{
	if (!Is32BitRIPRelativeJump(Address))
		return DefaultReturnValueOnFail;

	const uintptr_t TargetAddress = Resolve32BitRIPRelativeJumpTarget(Address);

	if (!Platform::IsAddressInProcessRange(TargetAddress))
		return DefaultReturnValueOnFail;

	return TargetAddress;
}

uintptr_t Architecture_x86_64::FindFunctionEnd(const uintptr_t Address, uint32_t Range)
{
	if (!Address)
		return NULL;

	if (Range > 0xFFFF)
		Range = 0xFFFF;

	for (int i = 0; i < Range; i++)
	{
		if (IsFunctionRet(Address + i))
			return Address + i;
	}

	return NULL;
}
uintptr_t Architecture_x86_64::FindNextFunctionStart(const uintptr_t Address)
{
	if (!Address)
		return NULL;

	const uintptr_t FuncEnd = FindFunctionEnd(Address);

	return FuncEnd % 0x10 != 0 ? FuncEnd + (0x10 - (FuncEnd % 0x10)) : FuncEnd;
}

uintptr_t Architecture_x86_64::FindNextFunctionStart(const void* Address)
{
	return FindNextFunctionStart(reinterpret_cast<uintptr_t>(Address));
}



uintptr_t Architecture_x86_64::GetRipRelativeCalledFunction(const uintptr_t Address, const int32_t OneBasedFuncIndex, bool(*IsWantedTarget)(const uintptr_t CalledAddr))
{
	if (!Address || OneBasedFuncIndex == 0)
		return NULL;

	const int32_t Multiply = OneBasedFuncIndex > 0 ? 1 : -1;

	/* Returns Index if FunctionIndex is positive, else -1 if the index is less than 0 */
	auto GetIndex = [=](const int32_t Index) -> int32_t { return Index * Multiply; };

	constexpr int32_t RealtiveCallOpcodeCount = 0x5;

	int32_t NumCalls = 0;

	const uint8_t* AsBytePtr = reinterpret_cast<const uint8_t*>(Address);

	for (int i = 0; i < 0xFFF; i++)
	{
		const int32_t Index = GetIndex(i);

		/* If this isn't a call, we don't care about it and want to continue */
		if (AsBytePtr[Index] != 0xE8)
			continue;

		const int32_t RelativeOffset = *reinterpret_cast<int32_t*>(Address + Index + 0x1 /* 0xE8 byte */);
		const uintptr_t RelativeCallTarget = Address + Index + RelativeOffset + RealtiveCallOpcodeCount;

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

	return NULL;
}