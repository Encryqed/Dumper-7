#pragma once
#include "Unreal/ObjectArray.h"
#include "HashStringTable.h"

enum class ECollisionType : uint8
{
	MemberName,
	SuperMemberName,
	FunctionName,
	SuperFunctionName,
	ParameterName,
	None,
};

inline std::string StringifyCollisionType(ECollisionType Type)
{
	switch (Type)
	{
	case ECollisionType::MemberName:
		return "ECollisionType::MemberName";
		break;
	case ECollisionType::SuperMemberName:
		return "ECollisionType::SuperMemberName";
		break;
	case ECollisionType::FunctionName:
		return "ECollisionType::FunctionName";
		break;
	case ECollisionType::SuperFunctionName:
		return "ECollisionType::SuperFunctionName";
		break;
	case ECollisionType::ParameterName:
		return "ECollisionType::ParameterName";
		break;
	case ECollisionType::None:
		return "ECollisionType::None";
		break;
	default:
		return "ECollisionType::Invalid";
		break;
	}
}

constexpr int32 OwnTypeBitCount = 0x7;
constexpr int32 PerCountBitCount = 0x5;

struct NameInfo
{
public:
	HashStringTableIndex Name;

	union
	{
		struct
		{
			uint32 OwnType : OwnTypeBitCount;

			// Order must match ECollisionType
			uint32 MemberNameCollisionCount : PerCountBitCount;
			uint32 SuperMemberNameCollisionCount : PerCountBitCount;
			uint32 FunctionNameCollisionCount : PerCountBitCount;
			uint32 SuperFuncNameCollisionCount : PerCountBitCount;
			uint32 ParamNameCollisionCount : PerCountBitCount;
		};

		uint32 CollisionData;
	};

	static_assert(sizeof(CollisionData) >= (OwnTypeBitCount + (5 * PerCountBitCount)) / 32, "Too many bits to fit into uint32, recude the number of bits!");

public:
	inline NameInfo()
		: Name(HashStringTableIndex::FromInt(-1)), CollisionData(0x0)
	{
	}

	NameInfo(HashStringTableIndex NameIdx, ECollisionType CurrentType);

public:
	void InitCollisionData(const NameInfo& Existing, ECollisionType CurrentType, bool bIsSuper);

	bool HasCollisions() const;

public:
	inline bool IsValid() const
	{
		return Name != -1;
	}

public:
	std::string DebugStringify() const;
};

namespace KeyFunctions
{
	/* Make a unique key from UEProperty/UEFunction for NameTranslation */
	uint64 GetKeyForCollisionInfo(UEStruct Super, UEProperty Member);
	uint64 GetKeyForCollisionInfo(UEStruct Super, UEFunction Function);
}

class CollisionManager
{
private:
	friend class CollisionManagerTest;
	friend class MemberManagerTest;

public:
	using NameContainer = std::vector<NameInfo>;

	using NameInfoMapType = std::unordered_map<uint64, NameContainer>;
	using TranslationMapType = std::unordered_map<uint64, uint64>;

private:
	/* Nametable used for storing the string-names of member-/function-names contained by NameInfos */
	HashStringTable MemberNames;

	/* Member-names and name-collision info*/
	CollisionManager::NameInfoMapType NameInfos;

	/* Map to translation from UEProperty/UEFunction to Index in NameContainer */
	CollisionManager::TranslationMapType TranslationMap;

	/* Names reserved for predefined members or local variables in function-bodies. Eg. "Class", "Parms", etc. */
	NameContainer ClassReservedNames;

	/* Names reserved for all members/parameters. Eg. "float", "operator", "return", ... */
	NameContainer ReservedNames;

private:
	/* Returns index of NameInfo inside of the NameContainer it was added to */
	uint64 AddNameToContainer(NameContainer& StructNames, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, bool bIsStruct, UEFunction Func = nullptr);

public:
	/* For external use by 'MemberManager::InitReservedNames()' */
	void AddReservedClassName(const std::string& Name, bool bIsParameterOrLocalVariable);
	void AddReservedName(const std::string& Name);
	void AddStructToNameContainer(UEStruct ObjAsStruct, bool bIsStruct);

	std::string StringifyName(UEStruct Struct, NameInfo Info);

public:
	template<typename UEType>
	inline NameInfo GetNameCollisionInfoUnchecked(UEStruct Struct, UEType Member)
	{
		CollisionManager::NameContainer& InfosForStruct = NameInfos.at(Struct.GetIndex());
		uint64 NameInfoIndex = TranslationMap[KeyFunctions::GetKeyForCollisionInfo(Struct, Member)];

		return InfosForStruct.at(NameInfoIndex);
	}
};

