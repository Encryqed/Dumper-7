#pragma once
#include <cstdint>

namespace Architecture_x86_64
{
	bool IsValid64BitVirtualAddress(const uintptr_t Address);
	bool IsValid64BitVirtualAddress(const void* Address);


	bool Is32BitRIPRelativeJump(const uintptr_t Address);

	uintptr_t Resolve32BitRIPRelativeJumpTarget(const uintptr_t Address);
	uintptr_t Resolve32BitRegisterRelativeJump(const uintptr_t Address);
	uintptr_t Resolve32BitSectionRelativeCall(const uintptr_t Address);
	uintptr_t Resolve32BitRelativeCall(const uintptr_t Address);
	uintptr_t Resolve32BitRelativeMove(const uintptr_t Address);
	uintptr_t Resolve32BitRelativeLea(const uintptr_t Address);
	uintptr_t Resolve32BitRelativePush(const uintptr_t Address);
	uintptr_t Resolve32bitAbsoluteCall(const uintptr_t Address);
	uintptr_t Resolve32bitAbsoluteMove(const uintptr_t Address);
}

