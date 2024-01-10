#include "DependencyManager.h"


DependencyManager::DependencyManager(int32 ObjectToTrack)
{
	AllDependencies.try_emplace(ObjectToTrack, IndexDependencyInfo{ });
}

void DependencyManager::AddDependency(const int32 DepedantIdx, int32 DependencyIndex)
{
	AllDependencies[DepedantIdx].DependencyIndices.insert(DependencyIndex);
}

void DependencyManager::SetDependencies(const int32 DepedantIdx, std::unordered_set<int32>&& Dependencies)
{
	AllDependencies[DepedantIdx].DependencyIndices = std::move(Dependencies);
}

size_t DependencyManager::GetNumEntries() const
{
	return AllDependencies.size();
}

void DependencyManager::VisitIndexAndDependencies(int32 Index, const IndexDependencyInfo& Data, IncludeFunctionType Callback) const
{
	auto& [bIsIncluded, Dependencies] = Data;

	if (!bIsIncluded)
	{
		bIsIncluded = true;

		for (int32 Dependency : Dependencies)
		{
			VisitIndexAndDependencies(Dependency, Data, Callback);
		}

		Callback(Index);
	}
}

void DependencyManager::VisitAllNodesWithCallback(IncludeFunctionType Callback) const
{
	for (const auto& [Index, DependencyInfo] : AllDependencies)
	{
		VisitIndexAndDependencies(Index, DependencyInfo, Callback);
	}
}
