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

	void SetDependencies(const int32 DepedantIdx, std::unordered_set<int32>&& Dependencies);

	size_t GetNumEntries() const;
};
