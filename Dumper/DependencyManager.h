#pragma once
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <format>

#include "Enums.h"


struct DependencyInfo
{
	bool bIsIncluded;
	bool bIsCyclic;

	/* 7 free bytes */

	std::unordered_set<int32> DependencyIndices;
};

class DependencyManager
{
	std::unordered_map<int32, DependencyInfo> AllDependencies;

public:
	DependencyManager() = default;

	DependencyManager(int32 ObjectToTrack)
	{
		AllDependencies.try_emplace(ObjectToTrack, DependencyInfo{ false, false, std::unordered_set<int32>{ } });
	}

public:
	inline void AddDependency(const int32 DepedantIdx, const int32 DependencyIndex)
	{
		AllDependencies[DepedantIdx].DependencyIndices.insert(DependencyIndex);
	}

	inline void SetDependencies(const int32 DepedantIdx, std::unordered_set<int32>&& Dependencies)
	{
		AllDependencies[DepedantIdx].DependencyIndices = std::move(Dependencies);
	}

	inline size_t GetNumEntries() const
	{
		return AllDependencies.size();
	}

	static void FindCyclicDependencies(DependencyManager& Nodes, int32 NodeIdx, int PrevNodeIdx, std::vector<int>& Visited)
	{
		auto& [bVisited, bIsCyclic, Dependencies] = Nodes.AllDependencies[NodeIdx];
	
		if (!bVisited)
		{
			bVisited = true;
	
			Visited.push_back(NodeIdx);
	
			for (int Dep : Dependencies)
				FindCyclicDependencies(Nodes, Dep, NodeIdx, Visited);
		}
		else
		{
			/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
			if (std::find(std::begin(Visited), std::end(Visited), NodeIdx) != std::end(Visited))
			{
				bIsCyclic = true;
				std::cout << std::format("Cycle between: {:d} and {:d}\n", PrevNodeIdx, NodeIdx);
			}
		}
	}
};