#pragma once
#include "UnrealTypes.h"

class FNameEntry
{
private:
	friend class NameArray;

private:
	static constexpr int32 NameWideMask = 0x1;

private:
	uint8* Address;

	static inline std::string(*GetStr)(uint8* NameEntry) = nullptr;

public:
	FNameEntry() = default;

	FNameEntry(void* Ptr);

public:
	std::string GetString();
	void* GetAddress();

private:
	static void Init();
};

class NameArray
{
private:
	static constexpr uint32 FNameBlockOffsetBits = 0x10;
	static constexpr uint32 FNameBlockMaxOffset = 0x1 << FNameBlockOffsetBits;

private:
	static uint8* GNames;

	static inline int64 NameEntryStride = 0x0;

	static inline void* (*ByIndex)(void* NamesArray, int32 ComparisonIndex) = nullptr;

private:
	static bool InitializeNameArray(uint8_t* NameArray);
	static bool InitializeNamePool(uint8_t* NamePool);

public:
	static void Init();

	static void Init(int32 GNamesOffset, bool bIsNamePool);
	
public:
	static int32 GetNumElements();
	static int32 GetNumChunks();

	static FNameEntry GetNameEntry(void* Name);
	static FNameEntry GetNameEntry(int32 Idx);
};
