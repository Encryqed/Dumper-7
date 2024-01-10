#pragma once
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <format>
#include <functional>

#include "Enums.h"


class DependencyManager
{
public:
	using IncludeFunctionType = std::function<void(int32 Index)>;

private:
	struct IndexDependencyInfo
	{
		mutable bool bIsIncluded;
		std::unordered_set<int32> DependencyIndices;
	};

private:
	std::unordered_map<int32, IndexDependencyInfo> AllDependencies;

public:
	DependencyManager() = default;

	DependencyManager(int32 ObjectToTrack);

private:
	void VisitIndexAndDependencies(int32 Index, const IndexDependencyInfo& Data, IncludeFunctionType Callback) const;

public:
	void AddDependency(const int32 DepedantIdx, int32 DependencyIndex);

	void SetDependencies(const int32 DepedantIdx, std::unordered_set<int32>&& Dependencies);

	size_t GetNumEntries() const;


	void VisitAllNodesWithCallback(IncludeFunctionType Callback) const;
};
