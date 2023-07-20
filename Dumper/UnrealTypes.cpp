#include "UnrealTypes.h"

FNamePool* FNamePool::Pool = nullptr;

void FNamePool::Init(int32 Offset)
{
	Pool = (FNamePool*)(reinterpret_cast<uintptr_t>(GetModuleHandle(0)) + Offset);
}

void(*FName::AppendString)(const void*, FString&) = nullptr;

