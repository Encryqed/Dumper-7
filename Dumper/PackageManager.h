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
using DependencyListType = std::vector<RequirementInfo>;


struct DependencyInfo
{
	IncludeStatus bIsIncluded;

	DependencyListType StructsDependencies;
	DependencyListType ClassesDependencies;
};

using VisitedNodeContainerType = std::vector<std::pair<int32, IncludeStatus>>;

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
		bool& bIsIncluded, bool bShouldInclude, bool bIsStruct) -> void
	{
		if (bShouldInclude && !bIsIncluded)
		{
			bIsIncluded = true;

			IncludeStatus Status = { bIsIncluded && bIsStruct, bIsIncluded && !bIsStruct };

			VisitedNodes.push_back({ CurrentIndex, Status });

			for (const RequirementInfo& Some : Dependencies)
			{
				NewParams.Requriements = Some;
				FindCycle(NewParams);
			}
		}
		else if (bShouldInclude && bIsIncluded)
		{
			/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
			if (std::find_if(VisitedNodes.begin(), VisitedNodes.end(), [&](const std::pair<int32, IncludeStatus>& Info)
			{
				return Info.first == CurrentIndex && (Info.second.Structs == bIsStruct || Info.second.Classes == !bIsStruct); /* Maybe wrong */
			}) != std::end(VisitedNodes))
			{
				//std::cout << "Found: " << Node << "\n";
				wprintf(L"Cycle between: %d and %d\n", CurrentIndex, PrevIndex);
			}
		}
	};

	DependencyInfo& Dependencies = Params.Nodes[Params.Requriements.PackageIdx];

	FindCycleHandler(Dependencies.StructsDependencies, Params.VisitedNodes, Params.Requriements.PackageIdx, Params.PrevNode, Dependencies.bIsIncluded.Structs, Params.Requriements.bShouldInclude.Structs, true);
	FindCycleHandler(Dependencies.ClassesDependencies, Params.VisitedNodes, Params.Requriements.PackageIdx, Params.PrevNode, Dependencies.bIsIncluded.Classes, Params.Requriements.bShouldInclude.Structs, false);

	if (Params.Requriements.bShouldInclude.Structs && !Dependencies.bIsIncluded.Structs)
	{
		Dependencies.bIsIncluded.Structs = true;

		Params.VisitedNodes.push_back({Params.Requriements.PackageIdx, Dependencies.bIsIncluded });

		for (const RequirementInfo& Some : Dependencies.StructsDependencies)
		{
			NewParams.Requriements = Some;
			FindCycle(NewParams);
		}
	}
	else if (Params.Requriements.bShouldInclude.Structs && Dependencies.bIsIncluded.Structs)
	{
		/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
		if (std::find_if(Params.VisitedNodes.begin(), Params.VisitedNodes.end(), [&](const std::pair<int32, IncludeStatus>& Info)
		{
			return Info.first == Params.Requriements.PackageIdx && Info.second.Structs == Params.Requriements.bShouldInclude.Structs;
		}) != std::end(Params.VisitedNodes))
		{
			//std::cout << "Found: " << Node << "\n";
			wprintf(L"Cycle between: %d and %d\n", Params.Requriements.PackageIdx, Params.PrevNode);
		}
	}

	if (Params.Requriements.bShouldIncludeClasses && !Dependencies.IncludeStatus.bIncludedClasses)
	{
		Dependencies.IncludeStatus.bIncludedClasses = true;

		Params.VisitedNodes.push_back({ Params.Requriements.PackageIdx, Dependencies.IncludeStatus });

		for (const RequirementInfo& Some : Dependencies.ClassesDependencies)
		{
			NewParams.Requriements = { Some.bShouldIncludeStructs, Some.bShouldIncludeClasses };
			FindCycle(NewParams);
		}
	}
	else if(Params.Requriements.bShouldIncludeClasses && Dependencies.IncludeStatus.bIncludedClasses)
	{
		/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
		if (std::find_if(Params.VisitedNodes.begin(), Params.VisitedNodes.end(), [&](const std::pair<int32, IncludeStatus>& Info)
		{
			return Info.first == Params.Requriements.PackageIdx && Info.second.bIncludedClasses == Params.Requriements.bShouldIncludeClasses;
		}) != std::end(Params.VisitedNodes))
		{
			//std::cout << "Found: " << Node << "\n";
			wprintf(L"Cycle between: %d and %d\n", Params.Requriements.PackageIdx, Params.PrevNode);
		}
	}
}


struct PackageInfo
{
private:
	friend class PackageInfoHandle;
	friend class PackageManager;

private:
	/* Name of this Package*/
	HashStringTableIndex Name;

	bool bHasClasses;
	bool bHasStructs;
	bool bHasFunctions;
	bool bHasEnums;

	DependencyManager Structs;
	DependencyManager Classes;

	DependencyInfo DependencyInfo;
};

constexpr int a = sizeof(PackageInfo);

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
	static void InitInternal();

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
