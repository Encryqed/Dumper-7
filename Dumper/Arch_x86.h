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

	bool IsFunctionRet(const uintptr_t Address);
	uintptr_t ResolveJumpIfInstructionIsJump(const uintptr_t Address, const uintptr_t DefaultReturnValueOnFail = NULL);

	/* Note: Unrealiable */
	uintptr_t FindNextFunctionStart(const uintptr_t Address);
	uintptr_t FindNextFunctionStart(const void* Address);

	uintptr_t FindFunctionEnd(const uintptr_t Address, uint32_t Range = 0xFFFF);

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
	uintptr_t GetRipRelativeCalledFunction(const uintptr_t Address, const int32_t OneBasedFuncIndex, bool(*IsWantedTarget)(const uintptr_t CalledAddr) = nullptr);
}

