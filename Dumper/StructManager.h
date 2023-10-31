#pragma once
#include <unordered_map>
#include "HashStringTable.h"
#include "UnrealObjects.h"

struct StructInfo
{
	int32 Size;
	HashStringTableIndex Name;
};

class StructManager
{
private:
	static inline HashStringTable UniqueNameTable;

	static inline std::unordered_map<int32 /*StructIdx*/, StructInfo> StructInfoOverrides;

public:
	static inline StructInfo GetInfo(UEStruct Struct)
	{
		return StructInfoOverrides[Struct.GetIndex()];
	}

	static inline const StringEntry& GetName(const StructInfo& Info)
	{
		return UniqueNameTable[Info.Name];
	}
};