#include "NameCollisionHandler.h"


NameInfo::NameInfo(HashStringTableIndex NameIdx, ECollisionType CurrentType)
	: Name(NameIdx), CollisionData(0x0)
{
	// Member-Initializer order is not guaranteed, init "OwnType" after
	OwnType = static_cast<uint8>(CurrentType);
}

void NameInfo::InitCollisionData(const NameInfo& Existing, ECollisionType CurrentType, bool bIsSuper)
{
	CollisionData = Existing.CollisionData;
	OwnType = static_cast<uint8>(CurrentType);

	// Increments the CollisionCount, for the CollisionType specified, by 1
	const int32 ShiftCount = OwnTypeBitCount + ((Existing.OwnType + bIsSuper) * PerCountBitCount);
	const int32 IncrementForCollisionCount = 1 << ShiftCount;
	CollisionData += IncrementForCollisionCount;
}

bool NameInfo::HasCollisions() const
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

	return false;
}

void CollisionManager::AddNameToContainer(std::unordered_map<int32, NameContainer>& Names, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, UEFunction Func)
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

void CollisionManager::AddStructToNameContainer(std::unordered_map<int32, NameContainer>& Names, HashStringTable& MemberNames, UEStruct ObjAsStruct)
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

std::string CollisionManager::StringifyName(HashStringTable& MemberNames, UEStruct Struct, NameInfo Info)
{
	ECollisionType OwnCollisionType = static_cast<ECollisionType>(Info.OwnType);

	std::string Name = MemberNames.GetStringEntry(Info.Name).GetName();

	//std::cout << "Nm: " << Name << "\n";

	// Order of sub-if-statements matters
	if (OwnCollisionType == ECollisionType::MemberName)
	{
		if (Info.SuperMemberNameCollisionCount > 0x0)
		{
			Name += ("_" + Struct.GetValidName());
		}
		if (Info.MemberNameCollisionCount > 0x0)
		{
			Name += ("_" + std::to_string(Info.MemberNameCollisionCount - 1));
		}
	}
	else if (OwnCollisionType == ECollisionType::FunctionName)
	{
		if (Info.MemberNameCollisionCount > 0x0 || Info.SuperMemberNameCollisionCount > 0x0)
		{
			Name = ("Func_" + Name);
		}
		if (Info.FunctionNameCollisionCount > 0x0)
		{
			Name += ("_" + std::to_string(Info.FunctionNameCollisionCount - 1));
		}
	}
	else if (OwnCollisionType == ECollisionType::ParameterName)
	{
		if (Info.MemberNameCollisionCount > 0x0 || Info.SuperMemberNameCollisionCount > 0x0 || Info.FunctionNameCollisionCount > 0x0 || Info.SuperFuncNameCollisionCount > 0x0)
		{
			Name = ("Param_" + Name);
		}
		if (Info.ParamNameCollisionCount > 0x0)
		{
			Name += ("_" + std::to_string(Info.ParamNameCollisionCount - 1));
		}
	}

	return Name;
}