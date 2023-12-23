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


PackageDependencyManager2::PackageDependencyManager2(int32 ObjectToTrack)
{
	AllDependencies.try_emplace(ObjectToTrack, DependencyInfoSet{ });
}

void PackageDependencyManager2::AddDependency(const int32 DepedantIdx, PackageDependencyInfo DependencyIndex)
{
	AllDependencies[DepedantIdx].insert(DependencyIndex);
}

void PackageDependencyManager2::SetDependencies(const int32 DepedantIdx, DependencyInfoSet&& Dependencies)
{
	AllDependencies[DepedantIdx] = std::move(Dependencies);
}

size_t PackageDependencyManager2::GetNumEntries() const
{
	return AllDependencies.size();
}

void PackageDependencyManager2::FindCyclicDependencies(PackageDependencyInfo Node, PackageDependencyInfo PrevNode, bool bIsVisitingStructsOnly, std::vector<int32>& Visited)
{
	//auto& [bIsIncluded, Dependencies] = AllDependencies[Node];
	//
	//PackageDependencyInfo& Info = Dependencies[Node];
	//
	//if (bIsVisitingStructsOnly >= Info.bRequiresStructs)
	//{
	//	bIsIncluded = true;
	//
	//	Visited.push_back(Info.DependencyPackageIdx);
	//
	//	for (PackageDependencyInfo& Dep : Dependencies)
	//		FindCyclicDependencies(Dep, Info, bIsVisitingStructsOnly, Visited);
	//}
	//
	//
	//
	//
	//if (!Info)
	//{
	//	bIsIncluded = true;
	//
	//	Visited.push_back(NodeIdx);
	//
	//	for (int Dep : Dependencies)
	//		FindCyclicDependencies<bIsVisitingStructs>(Dep, Info, Visited);
	//}
	//else
	//{
	//	/* No need to check unvisited nodes, they are guaranteed not to be in our "Visited" list */
	//	if (std::find(std::begin(Visited), std::end(Visited), NodeIdx) != std::end(Visited))
	//	{
	//		bIsCyclic = true;
	//		std::cout << std::format("Cycle between: {:d} and {:d}\n", PrevNodeIdx, NodeIdx);
	//	}
	//}
}
