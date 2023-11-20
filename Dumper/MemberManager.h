#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"

struct PredefMember
{
	std::string Type;
	std::string Name;
	int32 Offset;
	int32 Size;
	int32 ArrayDim;
	int32 Alignment;

	// Set by external code
	bool bHasUniqueName = true;
};

struct PredefinedFunction
{
	std::string RetType;
	std::string FuncName;
	std::vector<std::string> Params;

	// Set by external code
	bool bHasUniqueName = true;
};

class MemberManager
{
private:
	using MemberNameInfo = std::pair<HashStringTableIndex, bool /* bIsUnqiueInClassAndSuper */>;

public:
	static inline HashStringTable PropertyNames;
	static inline std::unordered_map<int32, std::vector<PredefMember>> Predefs;

	static inline std::unordered_map<int32, std::vector<MemberNameInfo>> StructMembers;

public:
	const std::vector<UEFunction> Functions; // No ptr
	const std::vector<PredefMember>* PredefinedFunctions = nullptr;
	const std::vector<UEProperty>* Properties = nullptr;
	const std::vector<PredefMember>* PredefinedMembers = nullptr;
	int32 PropertiesIdx = 0;
	int32 PredefMemberIdx = 0;

	MemberManager(UEStruct Struct, const std::vector<UEProperty>& Members)
		: Functions(Struct.GetFunctions()), Properties(&Members)
	{
		auto It = Predefs.find(Struct.GetIndex());
		if (It != Predefs.end())
			PredefinedMembers = &It->second;

		for (auto Property : Struct.GetProperties())
		{
			auto [Index, bIsUnique] = PropertyNames.FindOrAdd(Property.GetValidName());

			std::vector<MemberNameInfo>& Members = StructMembers[Struct.GetIndex()];

			if (bIsUnique)
			{
				Members.push_back({ Index, true });
				continue;
			}

			for (UEStruct S = Struct; S; S = S.GetSuper())
			{
				std::vector<MemberNameInfo>& SuperMembers = StructMembers[S.GetIndex()];
				
				if (std::find(SuperMembers.begin(), SuperMembers.end(), Index) != SuperMembers.end())
				{
					Members.push_back({ Index, false });
					break;
				}
			}
		}
	}

	static void AddStruct(UEStruct Struct)
	{
		std::vector<MemberNameInfo>& Members = StructMembers[Struct.GetAddress()];

		for (auto Property : Struct.GetProperties())
		{
			Members.push_back(PropertyNames.FindOrAdd(Property.GetValidName()));
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
