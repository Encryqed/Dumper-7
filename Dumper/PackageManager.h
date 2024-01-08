#pragma once

#include "Enums.h"

#include "UnrealObjects.h"
#include "DependencyManager.h"
#include "HashStringTable.h"


namespace PackageManagerUtils
{
	std::unordered_set<int32> GetDependencies(UEStruct Struct, int32 StructIndex);
}

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


struct PackageInfo
{
private:
	friend class PackageInfoHandle;
	friend class PackageManager;
	friend class PackageManagerTest;
	friend bool FindCycle(const struct FindCycleParams&, bool);

private:
	/* Name of this Package*/
	HashStringTableIndex Name;

	/* Count to track how many packages with this name already exists at the point this PackageInfos' initialization */
	uint8 CollisionCount = 0x0;

	bool bHasEnums;
	bool bHasStructs;
	bool bHasClasses;
	bool bHasFunctions;
	bool bHasParams;

	DependencyManager StructsSorted;
	DependencyManager ClassesSorted;

	std::vector<int32> Functions;
	std::vector<int32> Enums;

	DependencyInfo PackageDependencies;
};

struct FindCycleParams
{
	std::unordered_map<int32, PackageInfo>& Nodes;

	int32 PrevNode;
	bool bWasPrevNodeStructs;

	RequirementInfo Requriements;

	VisitedNodeContainerType& VisitedNodes;
};

bool FindCycle(const FindCycleParams& Params, bool bSuppressPrinting = false);


class PackageInfoHandle
{
private:
	const PackageInfo* Info;

public:
	PackageInfoHandle() = default;
	PackageInfoHandle(const PackageInfo& InInfo);

public:
	/* Returns a pair of name and CollisionCount */
	std::pair<std::string, uint8> GetName() const;
	const StringEntry& GetNameEntry() const;

	bool HasClasses() const;
	bool HasStructs() const;
	bool HasFunctions() const;
	bool HasParameterStructs() const;
	bool HasEnums() const;

	const DependencyManager& GetSortedStructs() const;
	const DependencyManager& GetSortedClasses() const;

	const std::vector<int32>& GetFunctions() const;
	const std::vector<int32>& GetEnums() const;

	const DependencyInfo& GetPackageDependencies() const;
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
	static void InitNameAndDependencies();

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

	static inline std::string GetName(int32 PackageIndex)
	{
		auto [NameString, Count] = GetInfo(PackageIndex).GetName();

		if (Count > 0) [[unlikely]]
			return NameString + "_" + std::to_string(Count - 1);

		return NameString;
	}

	static inline bool IsPackageNameUnique(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name].IsUnique();
	}

	static inline PackageInfoHandle GetInfo(int32 PackageIndex)
	{
		return PackageInfos.at(PackageIndex);
	}

	static inline PackageInfoHandle GetInfo(const UEObject Package)
	{
		if (!Package)
			return {};

		return PackageInfos.at(Package.GetIndex());
	}
};
