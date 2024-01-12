#include "PackageManager.h"
#include "ObjectArray.h"


inline void BooleanOrEqual(bool& b1, bool b2)
{
	b1 = b1 || b2;
}


PackageInfoHandle::PackageInfoHandle(const PackageInfo& InInfo)
	: Info(&InInfo)
{
}

const StringEntry& PackageInfoHandle::GetNameEntry() const
{
	return PackageManager::GetPackageName(*Info);
}

std::string PackageInfoHandle::GetName() const
{
	const StringEntry& Name = GetNameEntry();

	if (Info->CollisionCount <= 0) [[likely]]
		return Name.GetName();

	return Name.GetName() + "_" + std::to_string(Info->CollisionCount - 1);
}

std::pair<std::string, uint8> PackageInfoHandle::GetNameCollisionPair() const
{
	const StringEntry& Name = GetNameEntry();

	if (Name.IsUniqueInTable()) [[likely]]
		return { Name.GetName(), 0 };

	return { Name.GetName(), Info->CollisionCount };
}

bool PackageInfoHandle::HasClasses() const
{
	return Info->bHasClasses;
}

bool PackageInfoHandle::HasStructs() const
{
	return Info->bHasStructs;
}

bool PackageInfoHandle::HasFunctions() const
{
	return Info->bHasFunctions;
}

bool PackageInfoHandle::HasParameterStructs() const
{
	return Info->bHasParams;
}

bool PackageInfoHandle::HasEnums() const
{
	return Info->bHasEnums;
}

bool PackageInfoHandle::IsEmpty() const
{
	return !HasClasses() && !HasStructs() && !HasEnums() && !HasParameterStructs() && !HasFunctions();
}


const DependencyManager& PackageInfoHandle::GetSortedStructs() const
{
	return Info->StructsSorted;
}

const DependencyManager& PackageInfoHandle::GetSortedClasses() const
{
	return Info->ClassesSorted;
}

const std::vector<int32>& PackageInfoHandle::GetFunctions() const
{
	return Info->Functions;
}

const std::vector<int32>& PackageInfoHandle::GetEnums() const
{
	return Info->Enums;
}

const DependencyInfo& PackageInfoHandle::GetPackageDependencies() const
{
	return Info->PackageDependencies;
}


namespace PackageManagerUtils
{
	void GetPropertyDependency(UEProperty Prop, std::unordered_set<int32>& Store)
	{
		if (Prop.IsA(EClassCastFlags::StructProperty))
		{
			Store.insert(Prop.Cast<UEStructProperty>().GetUnderlayingStruct().GetIndex());
		}
		else if (Prop.IsA(EClassCastFlags::EnumProperty))
		{
			if (auto Enum = Prop.Cast<UEEnumProperty>().GetEnum())
				Store.insert(Enum.GetIndex());
		}
		else if (Prop.IsA(EClassCastFlags::ByteProperty))
		{
			if (UEObject Enum = Prop.Cast<UEByteProperty>().GetEnum())
				Store.insert(Enum.GetIndex());
		}
		else if (Prop.IsA(EClassCastFlags::ArrayProperty))
		{
			GetPropertyDependency(Prop.Cast<UEArrayProperty>().GetInnerProperty(), Store);
		}
		else if (Prop.IsA(EClassCastFlags::SetProperty))
		{
			GetPropertyDependency(Prop.Cast<UESetProperty>().GetElementProperty(), Store);
		}
		else if (Prop.IsA(EClassCastFlags::MapProperty))
		{
			GetPropertyDependency(Prop.Cast<UEMapProperty>().GetKeyProperty(), Store);
			GetPropertyDependency(Prop.Cast<UEMapProperty>().GetValueProperty(), Store);
		}
	}

	std::unordered_set<int32> GetDependencies(UEStruct Struct, int32 StructIndex)
	{
		std::unordered_set<int32> Dependencies;

		const int32 StructIdx = Struct.GetIndex();

		for (UEProperty Property : Struct.GetProperties())
		{
			GetPropertyDependency(Property, Dependencies);
		}

		Dependencies.erase(StructIdx);

		return Dependencies;
	};

	inline void SetPackageDependencies(DependencyListType& DependencyTracker, const std::unordered_set<int32>& Dependencies, int32 StructPackageIdx, bool bAllowToIncludeOwnPackage = false)
	{
		for (int32 Dependency : Dependencies)
		{
			const int32 PackageIdx = ObjectArray::GetByIndex(Dependency).GetPackageIndex();

			if (bAllowToIncludeOwnPackage || PackageIdx != StructPackageIdx)
				DependencyTracker[PackageIdx].bShouldIncludeStructs = true; // Dependencies only contains structs/enums which are in the "PackageName_structs.hpp" file
		}
	};

	inline void AddStructDependencies(DependencyManager& StructDependencies, const std::unordered_set<int32>& Dependenies, int32 StructIdx, int32 StructPackageIndex)
	{
		std::unordered_set<int32> TempSet;

		for (int32 DependencyStructIdx : Dependenies)
		{
			UEObject Obj = ObjectArray::GetByIndex(DependencyStructIdx);

			if (Obj.GetPackageIndex() == StructPackageIndex && !Obj.IsA(EClassCastFlags::Enum))
				TempSet.insert(DependencyStructIdx);
		}

		StructDependencies.SetDependencies(StructIdx, std::move(TempSet));
	};
}

void PackageManager::InitDependencies()
{
	// Collects all packages required to compile this file

	for (auto Obj : ObjectArray())
	{
		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject))
			continue;

		int32 CurrentPackageIdx = Obj.GetPackageIndex();

		const bool bIsStruct = Obj.IsA(EClassCastFlags::Struct);
		const bool bIsClass = Obj.IsA(EClassCastFlags::Class);

		const bool bIsFunction = Obj.IsA(EClassCastFlags::Function);
		const bool bIsEnum = Obj.IsA(EClassCastFlags::Enum);

		if (bIsStruct && !bIsFunction)
		{
			PackageInfo& Info = PackageInfos[CurrentPackageIdx];

			UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

			const int32 StructIdx = ObjAsStruct.GetIndex();
			const int32 StructPackageIdx = ObjAsStruct.GetPackageIndex();

			DependencyListType& PackageDependencyList = bIsClass ? Info.PackageDependencies.ClassesDependencies : Info.PackageDependencies.StructsDependencies;
			DependencyManager& ClassOrStructDependencyList = bIsClass ? Info.ClassesSorted : Info.StructsSorted;

			std::unordered_set<int32> Dependencies = PackageManagerUtils::GetDependencies(ObjAsStruct, StructIdx);

			ClassOrStructDependencyList.SetExists(StructIdx);

			PackageManagerUtils::SetPackageDependencies(PackageDependencyList, Dependencies, StructPackageIdx);

			if (!bIsClass)
				PackageManagerUtils::AddStructDependencies(ClassOrStructDependencyList, Dependencies, StructIdx, StructPackageIdx);

			/* for both struct and class */
			if (UEStruct Super = ObjAsStruct.GetSuper())
			{
				const int32 SuperPackageIdx = Super.GetPackageIndex();

				if (SuperPackageIdx == StructPackageIdx)
				{
					/* In-file sorting is only required if the super-class is inside of the same package */
					ClassOrStructDependencyList.AddDependency(Obj.GetIndex(), Super.GetIndex());
				}
				else
				{
					/* A package can't depend on itself, super of a structs will always be in _"structs" file, same for classes and "_classes" files */
					RequirementInfo& ReqInfo = PackageDependencyList[SuperPackageIdx];
					BooleanOrEqual(ReqInfo.bShouldIncludeStructs, !bIsClass);
					BooleanOrEqual(ReqInfo.bShouldIncludeClasses, bIsClass);
				}
			}

			BooleanOrEqual(Info.bHasStructs, bIsStruct);
			BooleanOrEqual(Info.bHasClasses, bIsClass);

			if (!bIsClass)
				continue;
			
			/* Add class-functions to package */
			for (UEFunction Func : ObjAsStruct.GetFunctions())
			{
				Info.bHasFunctions = true;
				Info.Functions.push_back(Obj.GetIndex());

				std::unordered_set<int32> Dependencies = PackageManagerUtils::GetDependencies(ObjAsStruct, StructIdx);

				BooleanOrEqual(Info.bHasParams, Func.HasMembers());

				PackageManagerUtils::SetPackageDependencies(Info.PackageDependencies.ParametersDependencies, Dependencies, StructPackageIdx, true);
			}
		}
		else if (bIsEnum)
		{
			PackageInfo& Info = PackageInfos[CurrentPackageIdx];

			Info.bHasEnums = true;
			Info.Enums.push_back(Obj.GetIndex());
		}
	}
}

void PackageManager::InitNames()
{
	for (auto& [PackageIdx, Info] : PackageInfos)
	{
		std::string PackageName = ObjectArray::GetByIndex(PackageIdx).GetValidName();

		auto [Name, bWasInserted] = UniquePackageNameTable.FindOrAdd(PackageName);
		Info.Name = Name;

		if (!bWasInserted) [[unlikely]]
			Info.CollisionCount = UniquePackageNameTable[Name].GetCollisionCount().CollisionCount;
	}
}

void PackageManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	PackageInfos.reserve(0x800);

	InitDependencies();
	InitNames();
}

template<bool bCheckForCycle>
void PackageManager::IterateDependenciesImplementation(const PackageManagerIterationParams& Params, const IteratePackagesCallbackType& CallbackForEachPackage, const FindCycleCallbackType& OnFoundCycle)
{
	PackageManagerIterationParams NewParams = {
		.PrevPackage = Params.RequiredPackge,
		.RequiredPackge = 4,

		.VisitedNodes = Params.VisitedNodes,
	};

	static auto FindCycleHandler = [&CallbackForEachPackage, &OnFoundCycle, OldParams = &Params](PackageManagerIterationParams& NewParams, const DependencyListType& Dependencies, VisitedNodeContainerType& VisitedNodes,
		int32 CurrentIndex, int32 PrevIndex, uint8& IterationHitCounter, bool bShouldHandlePackage, bool bIsStruct) -> void
	{
		if (!bShouldHandlePackage)
			return;

		const bool bIsIncluded = IterationHitCounter < CurrentIterationHitCount;

		if (bIsIncluded)
		{
			IterationHitCounter = CurrentIterationHitCount;

			const uint8 StructsHitCount = bIsStruct ? IterationHitCounter : 0x0;
			const uint8 ClassesHitCount = !bIsStruct ? IterationHitCounter : 0x0;

			VisitedNodes.push_back({ .PackageIdx = CurrentIndex, .StructsIterationHitCount = StructsHitCount, .ClassesIterationHitCount = ClassesHitCount, });

			bool bFoundCycle = false;

			for (auto& [Index, Requirements] : Dependencies)
			{
				NewParams.bWasPrevNodeStructs = bIsStruct;
				NewParams.bRequiresClasses = Requirements.bShouldIncludeClasses;
				NewParams.bRequiresStructs = Requirements.bShouldIncludeStructs;

				/* Iterate dependencies recursively */
				IterateDependenciesImplementation(NewParams, CallbackForEachPackage, OnFoundCycle);
			}

			// PERFORM ACTION
			CallbackForEachPackage(NewParams, *OldParams, bIsStruct);
		}

		if constexpr (bCheckForCycle)
		{
			if (!bIsIncluded)
				return;

			const bool bShouldIncludeStructs = bIsStruct;
			const bool bShouldIncludeClasses = !bIsStruct;

			auto CompareInfoPairs = [=](const VisitedNodeInformation& Info)
			{
				if (bShouldIncludeStructs)
					return Info.PackageIdx == CurrentIndex && Info.StructsIterationHitCount == IterationHitCounter;

				return Info.PackageIdx == CurrentIndex && Info.ClassesIterationHitCount == IterationHitCounter;
			};

			/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
			if (std::find_if(VisitedNodes.begin(), VisitedNodes.end(), CompareInfoPairs) != std::end(VisitedNodes))
			{
				OnFoundCycle(NewParams, *OldParams, bIsStruct);
			}
		}
	};

	DependencyInfo& Dependencies = PackageInfos[Params.RequiredPackge].PackageDependencies;

	FindCycleHandler(NewParams, Dependencies.StructsDependencies, Params.VisitedNodes, Params.RequiredPackge, Params.PrevPackage, Dependencies.StructsIterationHitCount, Params.bRequiresStructs, true);
	FindCycleHandler(NewParams, Dependencies.ClassesDependencies, Params.VisitedNodes, Params.RequiredPackge, Params.PrevPackage, Dependencies.ClassesIterationHitCount, Params.bRequiresClasses, false);
}

void PackageManager::IterateDependencies(const IteratePackagesCallbackType& CallbackForEachPackage)
{
	VisitedNodeContainerType VisitedNodes;

	PackageManagerIterationParams Params = {
		.PrevPackage = -1,

		.VisitedNodes = VisitedNodes,
	};

	FindCycleCallbackType OnCycleFoundCallback = [](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void { };

	for (const auto& [PackageIndex, Info] : PackageInfos)
	{
		Params.RequiredPackge = PackageIndex;
		Params.bWasPrevNodeStructs = true;
		Params.bRequiresClasses = true;
		Params.bRequiresStructs = true;
		Params.VisitedNodes.clear();

		IterateDependenciesImplementation(Params, CallbackForEachPackage, OnCycleFoundCallback);
	}
}

void PackageManager::FindCycle(const FindCycleCallbackType& OnFoundCycle)
{
	VisitedNodeContainerType VisitedNodes;

	PackageManagerIterationParams Params = {
		.PrevPackage = -1,

		.VisitedNodes = VisitedNodes,
	};

	FindCycleCallbackType CallbackForEachPackage = [](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void {};

	for (const auto& [PackageIndex, Info] : PackageInfos)
	{
		Params.RequiredPackge = PackageIndex;
		Params.bWasPrevNodeStructs = true;
		Params.bRequiresClasses = true;
		Params.bRequiresStructs = true;
		Params.VisitedNodes.clear();

		IterateDependenciesImplementation<true>(Params, CallbackForEachPackage, OnFoundCycle);
	}
}

