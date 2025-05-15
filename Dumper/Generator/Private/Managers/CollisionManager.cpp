#include "Managers/CollisionManager.h"


NameInfo::NameInfo(HashStringTableIndex NameIdx, ECollisionType CurrentType)
	: Name(NameIdx), CollisionData(0x0)
{
	// Member-Initializer order is not guaranteed, init "OwnType" after "CollisionData"
	OwnType = static_cast<uint8>(CurrentType);
}

void NameInfo::InitCollisionData(const NameInfo& Existing, ECollisionType CurrentType, bool bIsSuper)
{
	CollisionData = Existing.CollisionData;
	OwnType = static_cast<uint8>(CurrentType);

	switch (CurrentType)
	{
	case ECollisionType::MemberName:
		if (bIsSuper)
		{
			SuperMemberNameCollisionCount++;
			return;
		}
		MemberNameCollisionCount++;
		break;
	case ECollisionType::FunctionName:
		if (bIsSuper)
		{
			SuperFuncNameCollisionCount++;
			return;
		}
		FunctionNameCollisionCount++;
		break;
	case ECollisionType::ParameterName:
		ParamNameCollisionCount++;
		break;
	default:
		break;
	}
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

std::string NameInfo::DebugStringify() const
{
	return std::format(R"(
OwnType: {};
MemberNameCollisionCount: {};
SuperMemberNameCollisionCount: {};
FunctionNameCollisionCount: {};
SuperFuncNameCollisionCount: {};
ParamNameCollisionCount: {};
)", StringifyCollisionType(static_cast<ECollisionType>(OwnType)), MemberNameCollisionCount, SuperMemberNameCollisionCount, FunctionNameCollisionCount, SuperFuncNameCollisionCount, ParamNameCollisionCount);
}

uint64 KeyFunctions::GetKeyForCollisionInfo(UEStruct Super, UEProperty Member)
{
	uint64 Key = 0x0;

	FName Name = Member.GetFName();
	Key += Name.GetCompIdx();
	Key += Name.GetNumber();

	Key <<= 32;
	Key |= (static_cast<uint64>(Member.GetOffset()) << 24);
	Key |= (static_cast<uint64>(Member.GetSize()) << 16);
	Key |= Super.GetIndex();

	return reinterpret_cast<uint64>(Member.GetAddress());
}

uint64 KeyFunctions::GetKeyForCollisionInfo([[maybe_unused]] UEStruct Super, UEFunction Member)
{
	uint64 Key = 0x0;

	FName Name = Member.GetFName();
	Key += Name.GetCompIdx();
	Key += Name.GetNumber();

	Key <<= 32;
	Key |= Member.GetIndex();

	return Key;
}

uint64 CollisionManager::AddNameToContainer(NameContainer& StructNames, UEStruct Struct, std::pair<HashStringTableIndex, bool>&& NamePair, ECollisionType CurrentType, bool bIsStruct, UEFunction Func)
{
	static auto AddCollidingName = [](const NameContainer& SearchNames, NameContainer* OutTargetNames, HashStringTableIndex NameIdx, ECollisionType CurrentType, bool bIsSuper) -> bool
	{
		assert(OutTargetNames && "Target container was nullptr!");

		NameInfo NewInfo(NameIdx, CurrentType);
		NewInfo.OwnType = static_cast<uint8>(CurrentType);

		for (auto RevIt = SearchNames.crbegin(); RevIt != SearchNames.crend(); ++RevIt)
		{
			const NameInfo& ExistingName = *RevIt;

			if (ExistingName.Name != NameIdx)
				continue;

			NewInfo.InitCollisionData(ExistingName, CurrentType, bIsSuper);
			OutTargetNames->push_back(NewInfo);

			return true;
		}

		return false;
	};

	const bool bIsParameter = CurrentType == ECollisionType::ParameterName;

	auto [NameIdx, bWasInserted] = NamePair;

	if (bWasInserted && !bIsParameter)
	{
		// Create new empty NameInfo
		StructNames.emplace_back(NameIdx, CurrentType);
		return StructNames.size() - 1;
	}

	NameContainer* FuncParamNames = nullptr;

	if (Func)
	{
		FuncParamNames = &NameInfos[Func.GetIndex()];

		if (bWasInserted && bIsParameter)
		{
			// Create new empty NameInfo
			FuncParamNames->emplace_back(NameIdx, CurrentType);
			return FuncParamNames->size() - 1;
		}

		if (AddCollidingName(*FuncParamNames, FuncParamNames, NameIdx, CurrentType, false))
			return FuncParamNames->size() - 1;

		if (bIsStruct)
		{
			/* Serach ReservedNames last, just in case there was a property which also collided with a reserved name already */
			if (AddCollidingName(ReservedNames, FuncParamNames, NameIdx, CurrentType, false))
				return FuncParamNames->size() - 1;
		}
	}

	NameContainer* TargetNameContainer = bIsParameter ? FuncParamNames : &StructNames;

	/* Check all member-names from this struct and see if we're colliding with one of them */
	if (AddCollidingName(StructNames, TargetNameContainer, NameIdx, CurrentType, false))
		return TargetNameContainer->size() - 1;

	/* This possibly duplicated name doesn't occcure in the NameList of the struct itself, so check all supers to see if we're colliding with a super's name. */
	for (UEStruct Current = Struct.GetSuper(); Current; Current = Current.GetSuper())
	{
		NameContainer& SuperNames = NameInfos[Current.GetIndex()];

		if (AddCollidingName(SuperNames, TargetNameContainer, NameIdx, CurrentType, true))
			return TargetNameContainer->size() - 1;
	}

	if (!bIsStruct)
	{
		/* Serach ReservedNames last, just in case there was a predefined member of the super-class, or local variable, that collids with it. */
		if (AddCollidingName(ClassReservedNames, TargetNameContainer, NameIdx, CurrentType, false))
			return TargetNameContainer->size() - 1;
	}

	/* Serach ReservedNames last, just in case there was a property in the struct or parent struct, which also collided with a reserved name already */
	if (AddCollidingName(ReservedNames, TargetNameContainer, NameIdx, CurrentType, false))
		return TargetNameContainer->size() - 1;

	/* Searching this structs' name list, the super's name list, as well as ReservedNames did not yield any results. No collision on this name, add it! */
	if (bIsParameter && FuncParamNames)
	{
		FuncParamNames->emplace_back(NameIdx, CurrentType);
		return FuncParamNames->size() - 1;
	}
	else
	{
		StructNames.emplace_back(NameIdx, CurrentType);
		return StructNames.size() - 1;
	}
}

void CollisionManager::AddReservedClassName(const std::string& Name, bool bIsParameterOrLocalVariable)
{
	NameInfo NewInfo;
	NewInfo.Name = MemberNames.FindOrAdd(Name).first;
	NewInfo.CollisionData = 0x0;
	NewInfo.OwnType = static_cast<uint8>(bIsParameterOrLocalVariable ? ECollisionType::ParameterName : ECollisionType::SuperMemberName);

	ClassReservedNames.push_back(NewInfo);
}

void CollisionManager::AddReservedName(const std::string& Name)
{
	NameInfo NewInfo;
	NewInfo.Name = MemberNames.FindOrAdd(Name).first;
	NewInfo.CollisionData = 0x0;
	NewInfo.OwnType = static_cast<uint8>(ECollisionType::MemberName);

	ReservedNames.push_back(NewInfo);
}

void CollisionManager::AddStructToNameContainer(UEStruct Struct, bool bIsStruct)
{
	if (UEStruct Super = Struct.GetSuper())
	{
		if (NameInfos.find(Super.GetIndex()) == NameInfos.end())
			AddStructToNameContainer(Super, bIsStruct);
	}

	NameContainer& StructNames = NameInfos[Struct.GetIndex()];

	if (!StructNames.empty())
		return;

	auto AddToContainerAndTranslationMap = [&](auto Member, ECollisionType CollisionType, bool bIsStruct, UEFunction Func = nullptr) -> void
	{
		const uint64 Index = AddNameToContainer(StructNames, Struct, MemberNames.FindOrAdd(Member.GetValidName()), CollisionType, bIsStruct, Func);

		const auto [It, bInserted] = TranslationMap.emplace(KeyFunctions::GetKeyForCollisionInfo(Struct, Member), Index);
		
		if (!bInserted)
			std::cerr << "Error, no insertion took place, key {0x" << std::hex << KeyFunctions::GetKeyForCollisionInfo(Struct, Member) << "} duplicated!" << std::endl;
	};

	for (UEProperty Prop : Struct.GetProperties())
		AddToContainerAndTranslationMap(Prop, ECollisionType::MemberName, bIsStruct);

	for (UEFunction Func : Struct.GetFunctions())
	{
		AddToContainerAndTranslationMap(Func, ECollisionType::FunctionName, bIsStruct);

		for (UEProperty Prop : Func.GetProperties())
			AddToContainerAndTranslationMap(Prop, ECollisionType::ParameterName, bIsStruct, Func);
	}
};

std::string CollisionManager::StringifyName(UEStruct Struct, NameInfo Info)
{
	ECollisionType OwnCollisionType = static_cast<ECollisionType>(Info.OwnType);

	std::string Name = MemberNames.GetStringEntry(Info.Name).GetName();

	//std::cerr << "Nm: " << Name << "\nInfo:" << Info.DebugStringify() << "\n";

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