#pragma once
#include <unordered_map>
#include "HashStringTable.h"
#include "UnrealObjects.h"

class StructManager
{
private:
	static inline HashStringTable UniqueNameTable;
	static inline std::unordered_map<int32 /*StructIdx*/, uint32 /*RealSize*/> StructSizes;
	static inline std::unordered_map<int32 /*StructIdx*/, HashStringTableIndex /* UniqueName */> StructIdxToNameTranslations;

public:
	static inline int32 GetSize(UEStruct Struct)
	{
		int32 RetSize = Struct.GetStructSize();

		auto It = StructSizes.find(Struct.GetIndex());
		if (It != StructSizes.end())
			RetSize = It->second;

		return RetSize;
	}

	static inline const StringEntry& GetUniqueName(UEStruct Struct)
	{
		return UniqueNameTable.GetStringEntry(StructIdxToNameTranslations[Struct.GetIndex()]);
	}

	static inline std::string GetMemberUniqueName(UEStruct Struct, UEProperty Member)
	{
		return "GetMemberUniqueName isn't implemented yet!";
	}
};