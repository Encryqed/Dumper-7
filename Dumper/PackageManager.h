#pragma once

#include "Enums.h"

#include "UnrealObjects.h"
#include "DependencyManager.h"
#include "HashStringTable.h"

class PackageInfoHandle;
class PackageManager;

struct IncludeStatus
{
	bool Structs = false; // 'b' prefix omitted, it's the parents' responsibility here
	bool Classes = false; // 'b' prefix omitted, it's the parents' responsibility here
};


struct RequirementInfo
{
	int32 PackageIdx;
	IncludeStatus bShouldInclude;
};

struct VisitedNodeInformation
{
	int32 PackageIdx;
	IncludeStatus bIsIncluded;
};

using DependencyListType = std::unordered_map<int32, RequirementInfo>;


struct DependencyInfo
{
	IncludeStatus bIsIncluded;

	/* List of packages required by "ThisPackage_structs.h" */
	DependencyListType StructsDependencies;

	/* List of packages required by "ThisPackage_classes.h" */
	DependencyListType ClassesDependencies;

	/* List of packages required by "ThisPackage_params.h" */
	DependencyListType ParametersDependencies;
};

using VisitedNodeContainerType = std::vector<VisitedNodeInformation>;

struct FindCycleParams
{
	std::unordered_map<int32, DependencyInfo>& Nodes;

	int32 PrevNode;

	RequirementInfo Requriements;

	VisitedNodeContainerType& VisitedNodes;
};

void FindCycle(const FindCycleParams& Params)
{
	FindCycleParams NewParams = {
		.Nodes = Params.Nodes,
		.PrevNode = Params.Requriements.PackageIdx,
		/* Requriements */
		.VisitedNodes = Params.VisitedNodes,
	};

	static auto FindCycleHandler = [&NewParams](const DependencyListType& Dependencies, VisitedNodeContainerType& VisitedNodes,int32 CurrentIndex, int32 PrevIndex,
		bool& bIsIncluded, bool bShouldHandlePackage, bool bIsStruct) -> void
	{
		if (!bShouldHandlePackage)
			return;

		if (!bIsIncluded)
		{
			bIsIncluded = true;

			IncludeStatus Status = { bIsIncluded && bIsStruct, bIsIncluded && !bIsStruct };

			VisitedNodes.push_back({ CurrentIndex, Status });

			for (auto& [Index, bShouldInclude] : Dependencies)
			{
				NewParams.Requriements = { bIsStruct, !bIsStruct };
				NewParams.Requriements.PackageIdx = Index;
				FindCycle(NewParams);
			}
		}
		else if (bIsIncluded)
		{
			const bool bShouldIncludeStructs = bIsStruct;
			const bool bShouldIncludeClasses = !bIsStruct;

			auto CompareInfoPairs = [&](const VisitedNodeInformation& Info)
			{
				return Info.PackageIdx == CurrentIndex && ((Info.bIsIncluded.Structs && bShouldIncludeStructs) || (Info.bIsIncluded.Classes && bShouldIncludeClasses)); /* Maybe wrong */
			};

			/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
			if (std::find_if(VisitedNodes.begin(), VisitedNodes.end(), CompareInfoPairs) != std::end(VisitedNodes))
			{
				//std::cout << "Found: " << Node << "\n";
				wprintf(L"Cycle between: %d and %d\n", CurrentIndex, PrevIndex);
			}
		}
	};

	DependencyInfo& Dependencies = Params.Nodes[Params.Requriements.PackageIdx];

	FindCycleHandler(Dependencies.StructsDependencies, Params.VisitedNodes, Params.Requriements.PackageIdx, Params.PrevNode, Dependencies.bIsIncluded.Structs, Params.Requriements.bShouldInclude.Structs, true);
	FindCycleHandler(Dependencies.ClassesDependencies, Params.VisitedNodes, Params.Requriements.PackageIdx, Params.PrevNode, Dependencies.bIsIncluded.Classes, Params.Requriements.bShouldInclude.Structs, false);
}


struct PackageInfo
{
private:
	friend class PackageInfoHandle;
	friend class PackageManager;

private:
	/* Name of this Package*/
	HashStringTableIndex Name;

	bool bHasEnums;
	bool bHasStructs;
	bool bHasClasses;
	bool bHasFunctions;
	bool bHasParams;

	DependencyManager Structs;
	DependencyManager Classes;

	std::vector<int32> Functions;
	std::vector<int32> Enums;

	DependencyInfo PackageDependencies;
};


class PackageInfoHandle
{
private:
	const PackageInfo* Info;

public:
	PackageInfoHandle() = default;
	PackageInfoHandle(const PackageInfo& InInfo);

public:
	const StringEntry& GetName() const;

	bool HasClasses() const;
	bool HasStructs() const;
	bool HasFunctions() const;
	bool HasEnums() const;
};


class PackageManager
{
private:
	friend class PackageInfoHandle;
	friend class PackageManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /* PackageIndex */, PackageInfo>;

public:
	/* NameTable containing names of all Packages as well as information on name-collisions */
	static inline HashStringTable UniquePackageNameTable;

	/* Map containing infos on all Packages. Implemented due to information missing in the Unreal's reflection system (PackageSize). */
	static inline OverrideMaptType PackageInfos;

	static inline bool bIsInitialized = false;

private:
	static void InitPackageDependencies();
	static void InitStructDependenciesAndName();

public:
	static void Init();

private:
	static inline const StringEntry& GetPackageName(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name];
	}

public:
	static inline const OverrideMaptType& GetPackageInfos()
	{
		return PackageInfos;
	}

	static inline bool IsPackageNameUnique(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name].IsUnique();
	}

	static inline PackageInfoHandle GetInfo(const UEObject Package)
	{
		if (!Package)
			return {};

		return PackageInfos.at(Package.GetIndex());
	}
};
