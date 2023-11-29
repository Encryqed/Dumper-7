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

	bool bIsStatic;
};

struct PredefinedFunction
{
	std::string RetType;
	std::string FuncName;

	std::vector<std::string> Params;

	bool bIsStatic;
};

// todo: change name
struct PredefinedElements
{
	std::vector<PredefMember> Members;
	std::vector<PredefinedFunction> Functions;
};

class MemberManager
{
private:
	using MemberNameInfo = std::pair<HashStringTableIndex, bool /* bIsUnqiueInClassAndSuper */>;

public:
	static inline HashStringTable PropertyNames;
	static inline std::unordered_map<int32, PredefinedElements> Predefs;

	static inline std::unordered_map<int32, std::vector<MemberNameInfo>> StructMembers;

	// NEW
private:
	static inline std::unordered_map<int32 /* StructIndex*/, std::vector<uint64> /* NameInfos */> NameInfos;

private:
	std::vector<UEFunction> Functions;
	std::vector<UEProperty> Members;

	PredefinedElements* PredefElements;

	int32 CurrentFuncIdx = 0x0;
	int32 CurrentMemberIdx = 0x0;

	int32 CurrentPredefFuncIdx = 0x0;
	int32 CurrentPredefMemberIdx = 0x0;

	int32 NumStaticPredefMembers = 0x0;
	int32 NumStaticPredefFunctions = 0x0;

public:
	MemberManager(UEStruct Struct, const std::vector<UEProperty>& Members)
		: Functions(Struct.GetFunctions())
		, Members(Struct.GetProperties())
	{
		auto It = Predefs.find(Struct.GetIndex());
		if (It != Predefs.end())
		{
			PredefElements = &It->second;

			CurrentPredefFuncIdx = 0x0;
			CurrentPredefMemberIdx = 0x0;

			auto FindFirstStaticMember = [](auto& ElementVector) -> int32
			{
				for (int i = 0; i < ElementVector.size(); i++)
				{
					if (ElementVector[i].bIsStatic)
						return i + 1;
				}

				return 0x0;
			};

			NumStaticPredefMembers = FindFirstStaticMember(PredefElements->Members);
			NumStaticPredefFunctions = FindFirstStaticMember(PredefElements->Functions);
		}
	}

	inline int32 GetNumFunctions() const
	{
		return Functions.size();
	}

	inline int32 GetNumPredefFunctions() const
	{
		return PredefElements ? PredefElements->Functions.size() : 0x0;
	}

	inline int32 GetNumMembers() const
	{
		return Members.size();
	}

	inline int32 GetNumPredefMembers() const
	{
		return PredefElements ? PredefElements->Members.size() : 0x0;
	}

	inline bool HasNextMember() const
	{
		return GetNumFunctions() < CurrentFuncIdx || GetNumPredefFunctions() < CurrentPredefFuncIdx;
	}

	static void AddStruct(UEStruct Struct)
	{
	}

	inline std::string GetPropertyUniqueName()
	{
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
