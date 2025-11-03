#include "Managers/DependencyManager.h"

DependencyManager::DependencyManager( int32 ObjectToTrack )
{
	AllDependencies.try_emplace( ObjectToTrack, IndexDependencyInfo { } );
}

void DependencyManager::SetExists( const int32 DepedantIdx )
{
	AllDependencies [ DepedantIdx ];
}

void DependencyManager::AddDependency( const int32 DepedantIdx, int32 DependencyIndex )
{
	AllDependencies [ DepedantIdx ].DependencyIndices.insert( DependencyIndex );
}

void DependencyManager::SetDependencies( const int32 DepedantIdx, std::unordered_set<int32>&& Dependencies )
{
	AllDependencies [ DepedantIdx ].DependencyIndices = std::move( Dependencies );
}

size_t DependencyManager::GetNumEntries( ) const
{
	return AllDependencies.size( );
}

void DependencyManager::VisitIndexAndDependencies( int32 Index, OnVisitCallbackType Callback ) const
{
	// Use iterative approach instead of recursion to avoid stack overflow and improve performance
	std::vector<int32> Stack;
	std::vector<int32> PostOrder;
	Stack.reserve( 64 ); // Reserve space to reduce allocations
	PostOrder.reserve( AllDependencies.size( ) );

	Stack.push_back( Index );

	while ( !Stack.empty( ) )
	{
		int32 CurrentIndex = Stack.back( );
		Stack.pop_back( );

		auto It = AllDependencies.find( CurrentIndex );
		if ( It == AllDependencies.end( ) )
			continue;

		auto& [IterationHitCounter, Dependencies] = It->second;

		if ( IterationHitCounter >= CurrentIterationHitCount )
			continue;

		IterationHitCounter = CurrentIterationHitCount;
		PostOrder.push_back( CurrentIndex );

		// Add dependencies to stack
		for ( int32 Dependency : Dependencies )
		{
			Stack.push_back( Dependency );
		}
	}

	// Process in post-order (dependencies first)
	for ( auto It = PostOrder.rbegin( ); It != PostOrder.rend( ); ++It )
	{
		Callback( *It );
	}
}

void DependencyManager::VisitIndexAndDependenciesWithCallback( int32 Index, OnVisitCallbackType Callback ) const
{
	CurrentIterationHitCount++;

	VisitIndexAndDependencies( Index, Callback );
}

void DependencyManager::VisitAllNodesWithCallback( OnVisitCallbackType Callback ) const
{
	CurrentIterationHitCount++;

	for ( const auto& [Index, DependencyInfo] : AllDependencies )
	{
		VisitIndexAndDependencies( Index, Callback );
	}
}