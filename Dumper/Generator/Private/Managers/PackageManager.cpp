
#include "Unreal/ObjectArray.h"

#include "Managers/PackageManager.h"

/* Required for marking cyclic-headers in the StructManager */
#include "Managers/StructManager.h"


inline void BooleanOrEqual(bool& b1, bool b2)
{
	b1 = b1 || b2;
}

PackageInfoHandle::PackageInfoHandle(std::nullptr_t Nullptr)
	: Info(nullptr)
{
}

PackageInfoHandle::PackageInfoHandle(const PackageInfo& InInfo)
	: Info(&InInfo)
{
}

const StringEntry& PackageInfoHandle::GetNameEntry() const
{
	return PackageManager::GetPackageName(*Info);
}

int32 PackageInfoHandle::GetIndex() const
{
	return Info->PackageIndex;
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
	return Info->ClassesSorted.GetNumEntries() > 0x0;
}

bool PackageInfoHandle::HasStructs() const
{
	return Info->StructsSorted.GetNumEntries() > 0x0;
}

bool PackageInfoHandle::HasFunctions() const
{
	return !Info->Functions.empty();
}

bool PackageInfoHandle::HasParameterStructs() const
{
	return Info->bHasParams;
}

bool PackageInfoHandle::HasEnums() const
{
	return !Info->Enums.empty();
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

const std::vector<std::pair<int32, bool>>& PackageInfoHandle::GetEnumForwardDeclarations() const
{
	return Info->EnumForwardDeclarations;
}

const DependencyInfo& PackageInfoHandle::GetPackageDependencies() const
{
	return Info->PackageDependencies;
}

void PackageInfoHandle::ErasePackageDependencyFromStructs(int32 Package) const
{
	Info->PackageDependencies.StructsDependencies.erase(Package);
}

void PackageInfoHandle::ErasePackageDependencyFromClasses(int32 Package) const
{
	Info->PackageDependencies.ClassesDependencies.erase(Package);
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
			if (UEObject Enum = Prop.Cast<UEEnumProperty>().GetEnum())
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
		else if (Prop.IsA(EClassCastFlags::OptionalProperty) && !Prop.IsA(EClassCastFlags::ObjectPropertyBase))
		{
			GetPropertyDependency(Prop.Cast<UEOptionalProperty>().GetValueProperty(), Store);
		}
		else if (Prop.IsA(EClassCastFlags::DelegateProperty) || Prop.IsA(EClassCastFlags::MulticastInlineDelegateProperty))
		{
			const bool bIsNormalDeleage = !Prop.IsA(EClassCastFlags::MulticastInlineDelegateProperty);
			UEFunction SignatureFunction = bIsNormalDeleage ? Prop.Cast<UEDelegateProperty>().GetSignatureFunction() : Prop.Cast<UEMulticastInlineDelegateProperty>().GetSignatureFunction();

			if (!SignatureFunction)
				return;

			for (UEProperty DelegateParam : SignatureFunction.GetProperties())
			{
				GetPropertyDependency(DelegateParam, Store);
			}
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
	}

	inline void SetPackageDependencies(DependencyListType& DependencyTracker, const std::unordered_set<int32>& Dependencies, int32 StructPackageIdx, bool bAllowToIncludeOwnPackage = false)
	{
		for (int32 Dependency : Dependencies)
		{
			const int32 PackageIdx = ObjectArray::GetByIndex(Dependency).GetPackageIndex();


			if (bAllowToIncludeOwnPackage || PackageIdx != StructPackageIdx)
			{
				RequirementInfo& ReqInfo = DependencyTracker[PackageIdx];
				ReqInfo.PackageIdx = PackageIdx;
				ReqInfo.bShouldIncludeStructs = true; // Dependencies only contains structs/enums which are in the "PackageName_structs.hpp" file
			}
		}
	}

	inline void AddEnumPackageDependencies(DependencyListType& DependencyTracker, const std::unordered_set<int32>& Dependencies, int32 StructPackageIdx, bool bAllowToIncludeOwnPackage = false)
	{
		for (int32 Dependency : Dependencies)
		{
			UEObject DependencyObject = ObjectArray::GetByIndex(Dependency);

			if (!DependencyObject.IsA(EClassCastFlags::Enum))
				continue;

			const int32 PackageIdx = DependencyObject.GetPackageIndex();

			if (bAllowToIncludeOwnPackage || PackageIdx != StructPackageIdx)
			{
				RequirementInfo& ReqInfo = DependencyTracker[PackageIdx];
				ReqInfo.PackageIdx = PackageIdx;
				ReqInfo.bShouldIncludeStructs = true; // Dependencies only contains enums which are in the "PackageName_structs.hpp" file
			}
		}
	}

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
	}
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
			Info.PackageIndex = CurrentPackageIdx;

			UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

			const int32 StructIdx = ObjAsStruct.GetIndex();
			const int32 StructPackageIdx = ObjAsStruct.GetPackageIndex();

			DependencyListType& PackageDependencyList = bIsClass ? Info.PackageDependencies.ClassesDependencies : Info.PackageDependencies.StructsDependencies;
			DependencyManager& ClassOrStructDependencyList = bIsClass ? Info.ClassesSorted : Info.StructsSorted;

			std::unordered_set<int32> Dependencies = PackageManagerUtils::GetDependencies(ObjAsStruct, StructIdx);

			ClassOrStructDependencyList.SetExists(StructIdx);

			PackageManagerUtils::SetPackageDependencies(PackageDependencyList, Dependencies, StructPackageIdx, bIsClass);

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

			if (!bIsClass)
				continue;
			
			/* Add class-functions to package */
			for (UEFunction Func : ObjAsStruct.GetFunctions())
			{
				Info.Functions.push_back(Func.GetIndex());

				std::unordered_set<int32> ParamDependencies = PackageManagerUtils::GetDependencies(Func, Func.GetIndex());

				BooleanOrEqual(Info.bHasParams, Func.HasMembers());

				const int32 FuncPackageIndex = Func.GetPackageIndex();

				/* Add dependencies to ParamDependencies and add enums only to class dependencies (forwarddeclaration of enum classes defaults to int) */
				PackageManagerUtils::SetPackageDependencies(Info.PackageDependencies.ParametersDependencies, ParamDependencies, FuncPackageIndex, true);
				PackageManagerUtils::AddEnumPackageDependencies(Info.PackageDependencies.ClassesDependencies, ParamDependencies, FuncPackageIndex, true);
			}
		}
		else if (bIsEnum)
		{
			PackageInfo& Info = PackageInfos[CurrentPackageIdx];
			Info.PackageIndex = CurrentPackageIdx;

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

void PackageManager::HelperMarkStructDependenciesOfPackage(UEStruct Struct, int32 OwnPackageIdx, int32 RequiredPackageIdx, bool bIsClass)
{
	if (UEStruct Super = Struct.GetSuper())
	{
		if (Super.GetPackageIndex() == RequiredPackageIdx)
			StructManager::PackageManagerSetCycleForStruct(Super.GetIndex(), OwnPackageIdx);
	}

	if (bIsClass)
		return;

	for (UEProperty Child : Struct.GetProperties())
	{
		if (!Child.IsA(EClassCastFlags::StructProperty))
			continue;

		const UEStruct UnderlayingStruct = Child.Cast<UEStructProperty>().GetUnderlayingStruct();

		if (UnderlayingStruct.GetPackageIndex() == RequiredPackageIdx)
			StructManager::PackageManagerSetCycleForStruct(UnderlayingStruct.GetIndex(), OwnPackageIdx);
	}
}

int32 PackageManager::HelperCountStructDependenciesOfPackage(UEStruct Struct, int32 RequiredPackageIdx, bool bIsClass)
{
	int32 RetCount = 0x0;

	if (UEStruct Super = Struct.GetSuper())
	{
		if (Super.GetPackageIndex() == RequiredPackageIdx)
			RetCount++;
	}

	if (bIsClass)
		return RetCount;

	for (UEProperty Child : Struct.GetProperties())
	{
		if (!Child.IsA(EClassCastFlags::StructProperty))
			continue;

		const int32 UnderlayingStructPackageIdx = Child.Cast<UEStructProperty>().GetUnderlayingStruct().GetPackageIndex();

		if (UnderlayingStructPackageIdx == RequiredPackageIdx)
			RetCount++;
	}

	return RetCount;
}

void PackageManager::HelperAddEnumsFromPacakageToFwdDeclarations(UEStruct Struct, std::vector<std::pair<int32, bool>>& EnumsToForwardDeclare, int32 RequiredPackageIdx, bool bMarkAsClass)
{
	for (UEProperty Child : Struct.GetProperties())
	{
		const bool bIsEnumPrperty = Child.IsA(EClassCastFlags::EnumProperty);
		const bool bIsBytePrperty = Child.IsA(EClassCastFlags::ByteProperty);

		if (!bIsEnumPrperty && !bIsBytePrperty)
			continue;

		const UEObject UnderlayingEnum = bIsEnumPrperty ? Child.Cast<UEEnumProperty>().GetEnum() : Child.Cast<UEByteProperty>().GetEnum();

		if (UnderlayingEnum && UnderlayingEnum.GetPackageIndex() == RequiredPackageIdx)
			EnumsToForwardDeclare.emplace_back(UnderlayingEnum.GetIndex(), bMarkAsClass);
	}
}

void PackageManager::HelperInitEnumFwdDeclarationsForPackage(int32 PackageForFwdDeclarations, int32 RequiredPackage, bool bIsClass)
{
	PackageInfo& Info = PackageInfos.at(PackageForFwdDeclarations);

	std::vector<std::pair<int32, bool>>& EnumsToForwardDeclare = Info.EnumForwardDeclarations;

	DependencyManager::OnVisitCallbackType CheckForEnumsToForwardDeclareCallback = [&EnumsToForwardDeclare, RequiredPackage, bIsClass](int32 Index) -> void
	{
		HelperAddEnumsFromPacakageToFwdDeclarations(ObjectArray::GetByIndex<UEStruct>(Index), EnumsToForwardDeclare, RequiredPackage, bIsClass);
	};

	DependencyManager& Manager = bIsClass ? Info.ClassesSorted : Info.StructsSorted;
	Manager.VisitAllNodesWithCallback(CheckForEnumsToForwardDeclareCallback);

	/* Enums used in functions are required by classes too, due to the declaration of functions being in the classes-header */
	for (const int32 FuncIdx : Info.Functions)
	{
		HelperAddEnumsFromPacakageToFwdDeclarations(ObjectArray::GetByIndex<UEFunction>(FuncIdx), EnumsToForwardDeclare, RequiredPackage, true);
	}

	std::sort(EnumsToForwardDeclare.begin(), EnumsToForwardDeclare.end());
	EnumsToForwardDeclare.erase(std::unique(EnumsToForwardDeclare.begin(), EnumsToForwardDeclare.end()), EnumsToForwardDeclare.end());
}

/* Safe to use StructManager, initialization is guaranteed to have been finished */
void PackageManager::HandleCycles()
{
	struct CycleInfo
	{
		int32 CurrentPackage;
		int32 PreviousPacakge;

		bool bAreStructsCyclic;
		bool bAreclassesCyclic;
	};

	std::vector<CycleInfo> HandledPackages;


	FindCycleCallbackType CleanedUpOnCycleFoundCallback = [&HandledPackages](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void
	{
		const int32 CurrentPackageIndex = NewParams.RequiredPackage;
		const int32 PreviousPackageIndex = NewParams.PrevPackage;

		/* Check if this pacakge was handled before, return if true */
		for (const CycleInfo& Cycle : HandledPackages)
		{
			if (((Cycle.CurrentPackage == CurrentPackageIndex && Cycle.PreviousPacakge == PreviousPackageIndex)
				|| (Cycle.CurrentPackage == PreviousPackageIndex && Cycle.PreviousPacakge == CurrentPackageIndex))
				&& (Cycle.bAreStructsCyclic == bIsStruct || Cycle.bAreclassesCyclic == !bIsStruct))
			{
				return;
			}
		}

		/* Current cyclic packages will be added to 'HandledPackages' later on in this function */


		const PackageInfoHandle CurrentPackageInfo = GetInfo(CurrentPackageIndex);
		const PackageInfoHandle PreviousPackageInfo = GetInfo(PreviousPackageIndex);

		const DependencyManager& CurrentStructsOrClasses = bIsStruct ? CurrentPackageInfo.GetSortedStructs() : CurrentPackageInfo.GetSortedClasses();
		const DependencyManager& PreviousStructsOrClasses = bIsStruct ? PreviousPackageInfo.GetSortedStructs() : PreviousPackageInfo.GetSortedClasses();


		bool bIsMutualInclusion = false;

		if (bIsStruct)
		{
			bIsMutualInclusion = CurrentPackageInfo.GetPackageDependencies().StructsDependencies.contains(PreviousPackageIndex) 
				&& PreviousPackageInfo.GetPackageDependencies().StructsDependencies.contains(CurrentPackageIndex);
		}
		else
		{
			bIsMutualInclusion = CurrentPackageInfo.GetPackageDependencies().ClassesDependencies.contains(PreviousPackageIndex)
				&& PreviousPackageInfo.GetPackageDependencies().ClassesDependencies.contains(CurrentPackageIndex);
		}

		/* Use the number of dependencies between the packages to decide which one to mark as cyclic */
		if (bIsMutualInclusion)
		{
			/* Number of structs from PreviousPackage required by CurrentPackage */
			int32 NumStructsRequiredByCurrent = 0x0;

			DependencyManager::OnVisitCallbackType CountDependenciesForCurrent = [&NumStructsRequiredByCurrent, PreviousPackageIndex, bIsStruct](int32 Index) -> void
			{
				NumStructsRequiredByCurrent += HelperCountStructDependenciesOfPackage(ObjectArray::GetByIndex<UEStruct>(Index), PreviousPackageIndex, !bIsStruct);
			};
			CurrentStructsOrClasses.VisitAllNodesWithCallback(CountDependenciesForCurrent);


			/* Number of structs from CurrentPackage required by CurrentPackage PreviousPackage */
			int32 NumStructsRequiredByPrevious = 0x0;

			DependencyManager::OnVisitCallbackType CountDependenciesForPrevious = [&NumStructsRequiredByPrevious, CurrentPackageIndex, bIsStruct](int32 Index) -> void
			{
				NumStructsRequiredByPrevious += HelperCountStructDependenciesOfPackage(ObjectArray::GetByIndex<UEStruct>(Index), CurrentPackageIndex, !bIsStruct);
			};
			PreviousStructsOrClasses.VisitAllNodesWithCallback(CountDependenciesForPrevious);


			/* Which of the two cyclic packages requires less structs from the other package. */
			const bool bCurrentHasMoreDependencies = NumStructsRequiredByCurrent > NumStructsRequiredByPrevious;

			const int32 PackageIndexWithLeastDependencies = bCurrentHasMoreDependencies && bIsStruct ? PreviousPackageIndex : CurrentPackageIndex;
			const int32 PackageIndexToMarkCyclicWith = bCurrentHasMoreDependencies && bIsStruct ? CurrentPackageIndex : PreviousPackageIndex;


			/* Add package to HandledPackages */
			HandledPackages.push_back({ PackageIndexWithLeastDependencies, PackageIndexToMarkCyclicWith, bIsStruct, !bIsStruct });


			DependencyManager::OnVisitCallbackType SetCycleCallback = [PackageIndexWithLeastDependencies, PackageIndexToMarkCyclicWith, bIsStruct](int32 Index) -> void
			{
				HelperMarkStructDependenciesOfPackage(ObjectArray::GetByIndex<UEStruct>(Index), PackageIndexToMarkCyclicWith, PackageIndexWithLeastDependencies, !bIsStruct);
			};

			PreviousStructsOrClasses.VisitAllNodesWithCallback(SetCycleCallback);
		}
		else /* Just mark structs|classes from the previous package as cyclic */
		{
			HandledPackages.push_back({ PreviousPackageIndex, CurrentPackageIndex, bIsStruct, !bIsStruct });

			DependencyManager::OnVisitCallbackType SetCycleCallback = [PreviousPackageIndex, CurrentPackageIndex, bIsStruct](int32 Index) -> void
			{
				HelperMarkStructDependenciesOfPackage(ObjectArray::GetByIndex<UEStruct>(Index), PreviousPackageIndex, CurrentPackageIndex, !bIsStruct);
			};

			PreviousStructsOrClasses.VisitAllNodesWithCallback(SetCycleCallback);
		}
	};

	FindCycle(CleanedUpOnCycleFoundCallback);


	/* Actually remove the cycle form our dependency-graph. Couldn't be done before as it would've invalidated the iterator */
	for (const CycleInfo& Cycle : HandledPackages)
	{
		const PackageInfoHandle CurrentPackageInfo = GetInfo(Cycle.CurrentPackage);
		const PackageInfoHandle PreviousPackageInfo = GetInfo(Cycle.PreviousPacakge);

		/* Add enum forward declarations to the package from which we remove the dependency, as enums are not considered by those dependencies */
		HelperInitEnumFwdDeclarationsForPackage(Cycle.CurrentPackage, Cycle.PreviousPacakge, Cycle.bAreStructsCyclic);

		if (Cycle.bAreStructsCyclic)
		{
			CurrentPackageInfo.ErasePackageDependencyFromStructs(Cycle.PreviousPacakge);
			continue;
		}

		const RequirementInfo& CurrentRequirements = CurrentPackageInfo.GetPackageDependencies().ClassesDependencies.at(Cycle.CurrentPackage);

		/* Mark classes as 'do not include' when this package is cyclic but can still require _structs.hpp */
		if (CurrentRequirements.bShouldIncludeStructs)
		{
			const_cast<RequirementInfo&>(CurrentRequirements).bShouldIncludeClasses = false;
		}
		else
		{
			CurrentPackageInfo.ErasePackageDependencyFromClasses(Cycle.PreviousPacakge);
		}
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

void PackageManager::PostInit()
{
	if (bIsPostInitialized)
		return;

	bIsPostInitialized = true;

	StructManager::Init();

	HandleCycles();
}

void PackageManager::IterateSingleDependencyImplementation(SingleDependencyIterationParamsInternal& Params, bool bCheckForCycle)
{
	if (!Params.bShouldHandlePackage)
		return;

	const bool bIsIncluded = Params.IterationHitCounterRef >= CurrentIterationHitCount;

	if (!bIsIncluded)
	{
		Params.IterationHitCounterRef = CurrentIterationHitCount;

		IncludeData& Include = Params.VisitedNodes[Params.CurrentIndex];
		Include.bIncludedStructs = (Include.bIncludedStructs || Params.bIsStruct);
		Include.bIncludedClasses = (Include.bIncludedClasses || !Params.bIsStruct);

		for (auto& [Index, Requirements] : Params.Dependencies)
		{
			Params.NewParams.bWasPrevNodeStructs = Params.bIsStruct;
			Params.NewParams.bRequiresClasses = Requirements.bShouldIncludeClasses;
			Params.NewParams.bRequiresStructs = Requirements.bShouldIncludeStructs;
			Params.NewParams.RequiredPackage = Requirements.PackageIdx;

			/* Iterate dependencies recursively */
			IterateDependenciesImplementation(Params.NewParams, Params.CallbackForEachPackage, Params.OnFoundCycle, bCheckForCycle);
		}

		Params.VisitedNodes.erase(Params.CurrentIndex);

		// PERFORM ACTION
		Params.CallbackForEachPackage(Params.NewParams, Params.OldParams, Params.bIsStruct);
		return;
	}

	if (bCheckForCycle)
	{
		auto It = Params.VisitedNodes.find(Params.CurrentIndex);
		if (It != Params.VisitedNodes.end())
		{
			if ((It->second.bIncludedStructs && Params.bIsStruct) || (It->second.bIncludedClasses && !Params.bIsStruct))
				Params.OnFoundCycle(Params.NewParams, Params.OldParams, Params.bIsStruct);
		}
	}
}

void PackageManager::IterateDependenciesImplementation(const PackageManagerIterationParams& Params, const IteratePackagesCallbackType& CallbackForEachPackage, const FindCycleCallbackType& OnFoundCycle, bool bCheckForCycle)
{
	PackageManagerIterationParams NewParams = {
		.PrevPackage = Params.RequiredPackage,

		.VisitedNodes = Params.VisitedNodes,
	};

	DependencyInfo& Dependencies = PackageInfos.at(Params.RequiredPackage).PackageDependencies;

	SingleDependencyIterationParamsInternal StructsParams{
		.CallbackForEachPackage = CallbackForEachPackage,
		.OnFoundCycle = OnFoundCycle,

		.NewParams = NewParams,
		.OldParams = Params,
		.Dependencies = Dependencies.StructsDependencies,
		.VisitedNodes = Params.VisitedNodes,

		.CurrentIndex = Params.RequiredPackage,
		.PrevIndex = Params.PrevPackage,
		.IterationHitCounterRef = Dependencies.StructsIterationHitCount,

		.bShouldHandlePackage = Params.bRequiresStructs,
		.bIsStruct = true,
	};

	SingleDependencyIterationParamsInternal ClassesParams{
		.CallbackForEachPackage = CallbackForEachPackage,
		.OnFoundCycle = OnFoundCycle,

		.NewParams = NewParams,
		.OldParams = Params,
		.Dependencies = Dependencies.ClassesDependencies,
		.VisitedNodes = Params.VisitedNodes,

		.CurrentIndex = Params.RequiredPackage,
		.PrevIndex = Params.PrevPackage,
		.IterationHitCounterRef = Dependencies.ClassesIterationHitCount,

		.bShouldHandlePackage = Params.bRequiresClasses,
		.bIsStruct = false,
	};

	IterateSingleDependencyImplementation(StructsParams, bCheckForCycle);
	IterateSingleDependencyImplementation(ClassesParams, bCheckForCycle);
}

void PackageManager::IterateDependencies(const IteratePackagesCallbackType& CallbackForEachPackage)
{
	VisitedNodeContainerType VisitedNodes;

	PackageManagerIterationParams Params = {
		.PrevPackage = -1,

		.VisitedNodes = VisitedNodes,
	};

	FindCycleCallbackType OnCycleFoundCallback = [](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void { };

	/* Increment hit counter for new iteration-cycle */
	CurrentIterationHitCount++;

	for (const auto& [PackageIndex, Info] : PackageInfos)
	{
		Params.RequiredPackage = PackageIndex;
		Params.bWasPrevNodeStructs = true;
		Params.bRequiresClasses = true;
		Params.bRequiresStructs = true;
		Params.VisitedNodes.clear();

		IterateDependenciesImplementation(Params, CallbackForEachPackage, OnCycleFoundCallback, false);
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

	/* Increment hit counter for new iteration-cycle */
	CurrentIterationHitCount++;

	for (const auto& [PackageIndex, Info] : PackageInfos)
	{
		Params.RequiredPackage = PackageIndex;
		Params.bWasPrevNodeStructs = true;
		Params.bRequiresClasses = true;
		Params.bRequiresStructs = true;
		Params.VisitedNodes.clear();

		IterateDependenciesImplementation(Params, CallbackForEachPackage, OnFoundCycle, true);
	}
}

