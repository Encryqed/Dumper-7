#pragma once
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <format>

#include "Enums.h"


class DependencyManager
{
private:
	struct IndexDependencyInfo
	{
		bool bIsIncluded;
		std::unordered_set<int32> DependencyIndices;
	};

private:
	std::unordered_map<int32, IndexDependencyInfo> AllDependencies;

public:
	DependencyManager() = default;

	DependencyManager(int32 ObjectToTrack);

public:
	void AddDependency(const int32 DepedantIdx, int32 DependencyIndex);

	void SetDependencies(const int32 DepedantIdx, std::unordered_set<int32> Dependencies);

	size_t GetNumEntries() const;
};


struct PackageDependencyInfo
{
	int32 DependencyPackageIdx;

	uint8 bRequiresStructs : 1;
	uint8 bRequiresClasses : 1;

	uint8 bAreStructsCyclic : 1;
	uint8 bAreClassesCyclic : 1;

	uint8 bAreStructsIncluded : 1;
	uint8 bAreClassesIncluded : 1;

	inline operator int32() const { return DependencyPackageIdx; }
};

class PackageDependencyManager2
{
private:
	using DependencyInfoSet = std::unordered_set<PackageDependencyInfo, std::hash<int32>>;

private:
	std::unordered_map<int32, DependencyInfoSet> AllDependencies;

public:
	PackageDependencyManager2() = default;

	PackageDependencyManager2(int32 ObjectToTrack);

public:
	inline void AddDependency(const int32 DepedantIdx, PackageDependencyInfo DependencyIndex);

	inline void SetDependencies(const int32 DepedantIdx, DependencyInfoSet&& Dependencies);

	inline size_t GetNumEntries() const;

	void FindCyclicDependencies(PackageDependencyInfo Node, PackageDependencyInfo PrevNode, bool bIsVisitingStructsOnly, std::vector<int32>& Visited);
};
