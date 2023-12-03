#pragma once
#include "ObjectArray.h"
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
};

using NameContainer = std::vector<NameInfo>;

namespace CollisionManager
{
	void AddNameToContainer(std::unordered_map<int32, NameContainer>& Names, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, UEFunction Func = nullptr);

	void AddStructToNameContainer(std::unordered_map<int32, NameContainer>& Names, HashStringTable& MemberNames, UEStruct ObjAsStruct);

	std::string StringifyName(HashStringTable& MemberNames, UEStruct Struct, NameInfo Info);
};

