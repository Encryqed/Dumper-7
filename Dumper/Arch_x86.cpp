#pragma once
#include "Arch_x86.h"

// The processor (x86-64) only translates 52bits (or 57 bits) of a virtual address into a physical address and the unused bits need to be all 0 or all 1.
bool Architecture_x86_64::IsValidVirtualAddress(const uintptr_t Address)
{
	constexpr uint64_t BitMask = 0b1111'1111ull << 56;

	return (Address & BitMask) == BitMask || (Address & BitMask) == 0x0;
}

bool Architecture_x86_64::IsValidVirtualAddress(const void* Address)
{
	return IsValidVirtualAddress(reinterpret_cast<const uintptr_t>(Address));
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

