#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"

/*
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

public:
	NameInfo(HashStringTableIndex NameIdx, ECollisionType CurrentType)
		: Name(NameIdx), CollisionData(0x0)
	{
		// Member-Initializer order is not guaranteed, init "OwnType" after
		OwnType = static_cast<uint8>(CurrentType);
	}

public:
	inline void InitCollisionData(const NameInfo& Existing, ECollisionType CurrentType, bool bIsSuper)
	{
		CollisionData = Existing.CollisionData;
		OwnType = static_cast<uint8>(CurrentType);

		// Increments the CollisionCount, for the CollisionType specified, by 1
		const int32 ShiftCount = OwnTypeBitCount + ((Existing.OwnType + bIsSuper) * PerCountBitCount);
		const int32 IncrementForCollisionCount = 1 << ShiftCount;
		CollisionData += IncrementForCollisionCount;
	}

	inline bool IsValid() const
	{
		return Name != -1;
	}

	inline bool HasCollisions() const
	{
		ECollisionType OwnCollisionType = static_cast<ECollisionType>(OwnType);

		if (OwnCollisionType == ECollisionType::MemberName)
		{
			return SuperMemberNameCollisionCount > 0x0 || MemberNameCollisionCount > 0x0;
		}
		else if (OwnCollisionType == ECollisionType::FunctionName)
		{
			return MemberNameCollisionCount > 0x0 || SuperMemberNameCollisionCount > 0x0 || FunctionNameCollisionCount > 0x0;
		}
		else if (OwnCollisionType == ECollisionType::ParameterName)
		{
			return MemberNameCollisionCount > 0x0 || SuperMemberNameCollisionCount > 0x0 || FunctionNameCollisionCount > 0x0 || SuperFuncNameCollisionCount > 0x0 || ParamNameCollisionCount > 0x0;
		}
	}
};

using NameContainer = std::vector<NameInfo>;
*/
inline uint64 TempGlobalNameCounter = 0x0;

void AddNameToContainer(std::unordered_map<int32, NameContainer>& Names, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, UEFunction Func = nullptr)
{
	static auto AddCollidingName = [](const NameContainer& SearchNames, NameContainer* OutTargetNames, HashStringTableIndex NameIdx, ECollisionType CurrentType, bool bIsSuper) -> bool
	{
		assert(OutTargetNames && "Target container was nullptr!");

		NameInfo NewInfo(NameIdx, CurrentType);
		NewInfo.OwnType = static_cast<uint8>(CurrentType);

		for (const NameInfo& ExistingName : SearchNames)
		{
			if (ExistingName.Name != NameIdx)
				continue;

			NewInfo.InitCollisionData(ExistingName, CurrentType, bIsSuper);
			OutTargetNames->push_back(NewInfo);

			return true;
		}

		return false;
	};

	TempGlobalNameCounter++;

	NameContainer& StructNames = Names[Struct.GetIndex()];

	const bool bIsParameter = CurrentType == ECollisionType::ParameterName;

	auto [NameIdx, bWasInserted] = NamePair;

	if (bWasInserted && !bIsParameter)
	{
		// Create new empty NameInfo
		StructNames.emplace_back(NameIdx, CurrentType);
		return;
	}

	NameContainer* FuncParamNames = nullptr;

	if (Func)
	{
		FuncParamNames = &Names[Func.GetIndex()];

		if (bWasInserted)
		{
			// Create new empty NameInfo
			FuncParamNames->emplace_back(NameIdx, CurrentType);
			return;
		}

		if (AddCollidingName(*FuncParamNames, FuncParamNames, NameIdx, CurrentType, false))
			return;
	}

	NameContainer* TargetNameContainer = bIsParameter ? FuncParamNames : &StructNames;

	if (AddCollidingName(StructNames, TargetNameContainer, NameIdx, CurrentType, false))
		return;

	for (UEStruct Current = Struct.GetSuper(); Current; Current = Current.GetSuper())
	{
		NameContainer& SuperNames = Names[Current.GetIndex()];

		if (AddCollidingName(SuperNames, TargetNameContainer, NameIdx, CurrentType, true))
			return;
	}

	// Create new empty NameInfo
	if (bIsParameter && FuncParamNames)
	{
		FuncParamNames->emplace_back(NameIdx, CurrentType);
	}
	else
	{
		StructNames.emplace_back(NameIdx, CurrentType);
	}
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

	std::cout << std::endl;
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

		for (auto [StructIdx, NameContainer] : Names)
		{
			const UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);
			const bool bIsFunction = Struct.IsA(EClassCastFlags::Function);

			NumNames += NameContainer.size();

			for (auto NameInfo : NameContainer)
			{
				if (!bIsFunction && static_cast<ECollisionType>(NameInfo.OwnType) == ECollisionType::ParameterName)
					std::cout << std::format("Invalid: Param in '{}'\n", Struct.GetName());

				if (NameInfo.HasCollisions())
					NumCollidingNames++;
			}
		}

		PrintCollidingNames(Names, MemberNames);

		std::cout << "GlobalNumNames: " << TempGlobalNameCounter << "\n" << std::endl;

		std::cout << "NumNames: " << NumNames << std::endl;
		std::cout << "NumCollidingNames: " << NumCollidingNames << std::endl;

		std::cout << std::format("Collisions in '%': {:.04f}%\n", static_cast<double>(NumCollidingNames) / NumNames) << std::endl;
	}
};
