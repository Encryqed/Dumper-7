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

struct CollisionPair
{
	ECollisionType NameType;
	ECollisionType TargetType;
};

constexpr int32 OwnTypeBitCount = 0x7;
constexpr int PerCountBitCount = 0x5;

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

public:
	NameInfo(HashStringTableIndex NameIdx, ECollisionType CurrentType)
		: Name(NameIdx), CollisionData(0x0)
	{
		// Member-Initializer order is not guaranteed, init "OwnType" after
		OwnType = static_cast<uint8>(CurrentType);
	}

public:
	inline void InitCollisionData(NameInfo& Existing, ECollisionType CurrentType, bool bIsSuper)
	{
		CollisionData = Existing.CollisionData;
		OwnType = static_cast<uint8>(CurrentType);

		// Increments the CollisionCount, for the CollisionType specified, by 1
		const int32 ShiftCount = OwnTypeBitCount + ((static_cast<uint8>(OwnType) + bIsSuper) * PerCountBitCount);
		const int32 IncrementForCollisionCount = 1 << ShiftCount;
		CollisionData += IncrementForCollisionCount;
	}

	inline bool IsValid() const
	{
		return Name != -1;
	}

	inline bool HasCollisions() const
	{
		return (CollisionData >> OwnTypeBitCount) > 0x0;
	}
};

using NameContainer = std::vector<NameInfo>;

inline uint64 TempGlobalNameCounter = 0x0;

void AddNameToContainer(std::unordered_map<int32, NameContainer>& Names, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, UEFunction Func = nullptr)
{
	static auto AddCollidingName = [](NameContainer& Container, HashStringTableIndex NameIdx, ECollisionType CurrentType, bool bIsSuper) -> bool
	{
		NameInfo NewInfo(NameIdx, CurrentType);
		NewInfo.OwnType = static_cast<uint8>(CurrentType);

		for (NameInfo& ExistingName : Container)
		{
			if (ExistingName.Name != NameIdx)
				continue;

			NewInfo.InitCollisionData(ExistingName, CurrentType, bIsSuper);
			Container.push_back(NewInfo);

			return true;
		}

		return false;
	};

	TempGlobalNameCounter++;

	NameContainer& Container = Names[Struct.GetIndex()];

	auto [NameIdx, bWasInserted] = NamePair;

	if (bWasInserted)
	{
		// Create new empty NameInfo
		Container.emplace_back(NameIdx, CurrentType);
		return;
	}

	if (Func)
	{
		NameContainer& Container = Names[Func.GetIndex()];

		if (AddCollidingName(Container, NameIdx, CurrentType, false))
			return;
	}

	if (AddCollidingName(Container, NameIdx, CurrentType, false))
		return;

	for (UEStruct Current = Struct.GetSuper(); Current; Current = Current.GetSuper())
	{
		NameContainer& SuperContainer = Names[Current.GetIndex()];

		if (AddCollidingName(SuperContainer, NameIdx, CurrentType, true))
			return;
	}

	// Create new empty NameInfo
	Container.emplace_back(NameIdx, CurrentType);
}

inline void AddStructToNameContainer(std::unordered_map<int32, NameContainer>& Names, HashStringTable& MemberNames, UEStruct ObjAsStruct)
{
	if (UEStruct Super = ObjAsStruct.GetSuper())
	{
		if (Names.find(Super.GetIndex()) == Names.end())
			AddStructToNameContainer(Names, MemberNames, Super);
	}

	//const NameContainer& Infos = Names[ObjAsStruct.GetIndex()];
	for (UEProperty Prop : ObjAsStruct.GetProperties())
		AddNameToContainer(Names, ObjAsStruct, MemberNames.FindOrAdd(Prop.GetValidName()), ECollisionType::MemberName);

	for (UEFunction Func : ObjAsStruct.GetFunctions())
	{
		AddNameToContainer(Names, ObjAsStruct, MemberNames.FindOrAdd(Func.GetValidName()), ECollisionType::FunctionName);

		for (UEProperty Prop : Func.GetProperties())
			AddNameToContainer(Names, ObjAsStruct, MemberNames.FindOrAdd(Prop.GetValidName()), ECollisionType::ParameterName, Func);
	}
};

inline void PrintCollidingNames(std::unordered_map<int32, NameContainer>& Names, HashStringTable& MemberNames)
{
	for (auto [StructIdx, NameContainer] : Names)
	{
		UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

		for (auto NameInfo : NameContainer)
		{
			if (!NameInfo.HasCollisions())
				continue;

			ECollisionType OwnCollisionType = static_cast<ECollisionType>(NameInfo.OwnType);

			std::string Name = MemberNames.GetStringEntry(NameInfo.Name).GetName();

			//std::cout << "Nm: " << Name << "\n";

			// Order of sub-if-statements matters
			if (OwnCollisionType == ECollisionType::MemberName)
			{
				if (NameInfo.SuperMemberNameCollisionCount > 0x0)
				{
					Name += ("_" + Struct.GetValidName());
					std::cout << "SMCnt-";
				}
				if (NameInfo.MemberNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.MemberNameCollisionCount - 1));
					std::cout << "MbmCnt-";
				}
			}
			else if (OwnCollisionType == ECollisionType::FunctionName)
			{
				if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0)
				{
					Name = ("Func_" + Name);
					std::cout << "Fnc-";
				}
				if (NameInfo.FunctionNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.FunctionNameCollisionCount - 1));
					std::cout << "FncCnt-";
				}
			}
			else if (OwnCollisionType == ECollisionType::ParameterName)
			{
				if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0 || NameInfo.FunctionNameCollisionCount > 0x0 || NameInfo.SuperFuncNameCollisionCount > 0x0)
				{
					Name = ("Param_" + Name);
					std::cout << "Prm-";
				}
				if (NameInfo.ParamNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.ParamNameCollisionCount - 1));
					std::cout << "PrmCnt-";
				}
			}

			std::cout << "Name: " << Name << "\n";
		}
	}
}

class UniqueNameTest
{
public:
	static inline void TestMakeNamesUnique()
	{
		HashStringTable MemberNames;

		std::unordered_map<int32, NameContainer> Names;

		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			AddStructToNameContainer(Names, MemberNames, Obj.Cast<UEStruct>());
		}

		uint64 NumNames = 0x0;
		uint64 NumCollidingNames = 0x0;

		for (auto [_, NameContainer] : Names)
		{
			NumNames += NameContainer.size();

			for (auto NameInfo : NameContainer)
			{
				if (NameInfo.HasCollisions())
					NumCollidingNames++;
			}
		}

		PrintCollidingNames(Names, MemberNames);

		std::cout << "GlobalNumNames: " << TempGlobalNameCounter << "\n" << std::endl;

		std::cout << "NumNames: " << NumNames << std::endl;
		std::cout << "NumCollidingNames: " << NumCollidingNames << std::endl;

		std::cout << std::format("Collisions in '%': {:.03f}%\n", static_cast<double>(NumCollidingNames) / NumNames) << std::endl;
	}
};
