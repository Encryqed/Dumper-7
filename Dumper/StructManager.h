#pragma once
#include <unordered_map>
#include "HashStringTable.h"
#include "UnrealObjects.h"


/*
Struct 1:
	int a, b, c;
	uint8 Pad[0x10];
	int d, e, f;

Struct1_PredefinedMembers: {
	"void*", "Something", 0x10, 0x8
}

Result:
struct Struct1
{
public:
	int32                 a;                              // 0x0000(0x0004)
	int32                 b;                              // 0x0004(0x0004)
	int32                 c;                              // 0x0008(0x0004)
	uint8                 Pad_0[0x4];                     // 0x000C(0x0004)
	void*                 Something;                      // 0x0010(0x0008)
	uint8                 Pad_1[0x4];                     // 0x0018(0x0004)
	int32                 e;                              // 0x001C(0x0004)
	int32                 f;                              // 0x0020(0x0004)
};
*/

struct TestPredefMember
{
	std::string Type;
	std::string Name;
	int32 Offset;
	int32 Size;
	int32 ArrayDim;
	int32 Alignment;
	HashStringTableIndex NameIndexIfDuplicated;
};

class MemberManager
{
private:
	using MemberNameInfo = std::pair<HashStringTableIndex, bool /* bIsUnqiueInClassAndSuper */>;

public:
	static inline HashStringTable PropertyNames;
	static inline std::unordered_map<void*, std::vector<TestPredefMember>> Predefs;

	static inline std::unordered_map<void*, std::vector<MemberNameInfo>> StructMembers;

public:
	const std::vector<UEProperty>* Properties = nullptr;
	const std::vector<TestPredefMember>* PredefinedMembers = nullptr;
	int32 PropertiesIdx = 0;
	int32 PredefMemberIdx = 0;

	MemberManager(UEStruct Struct, const std::vector<UEProperty>& Members)
		: Properties(&Members)
	{
		auto It = Predefs.find(Struct.GetAddress());
		if (It != Predefs.end())
			PredefinedMembers = &It->second;
		
		for (auto Property : Struct.GetProperties())
		{
			auto [Index, bIsUnique] = PropertyNames.FindOrAdd(Property.GetValidName(), true);
		
			std::vector<MemberNameInfo>& Members = StructMembers[Struct.GetAddress()];

			if (bIsUnique)
			{
				Members.push_back({ Index, true });
				continue;
			}

			for (UEStruct S = Struct; S; S = S.GetSuper())
			{
				//std::vector<MemberNameInfo>& SuperMembers = StructMembers[S.GetAddress()];
				//
				//if (std::find(SuperMembers.begin(), SuperMembers.end(), Index) != SuperMembers.end())
				//{
				//	Members.push_back({ Index, false });
				//	break;
				//}
			}
		}
	}

	static void AddStruct(UEStruct Struct)
	{
		std::vector<MemberNameInfo>& Members = StructMembers[Struct.GetAddress()];

		for (auto Property : Struct.GetProperties())
		{
			Members.push_back(PropertyNames.FindOrAdd(Property.GetValidName(), true));
		}
	}

	inline std::string GetPropertyUniqueName()
	{
		//return 
	}

public:
	static inline HashStringTable& GetStringTable()
	{
		return PropertyNames;
	}

	inline UEProperty GetNextProperty() const
	{
		//if (PredefinedMembers && PredefinedMembers->at(PredefMemberIdx).Offset)
	}
};

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

