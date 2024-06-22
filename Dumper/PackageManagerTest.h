#pragma once
#include "HashStringTable.h"
#include "PackageManager.h"
#include "TestBase.h"
#include "ObjectArray.h"

#include <cassert>


class PackageManagerTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestInit<bDoDebugPrinting>();
		TestInfo<bDoDebugPrinting>();
		TestIncludeTypes<bDoDebugPrinting>();
		TestMutliIterations<bDoDebugPrinting>();
		TestFindCyclidDependencies<bDoDebugPrinting>();
		TestCyclicDependencyDetection<bDoDebugPrinting>();
		TestUniquePackageNameGeneration<bDoDebugPrinting>();
		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInit()
	{
		PackageManager::Init();

		int32 FirstInitSize = PackageManager::PackageInfos.size();

		PackageManager::Init();
		PackageManager::Init();
		PackageManager::Init();

		bool bSuccededTestWithoutError = PackageManager::PackageInfos.size() == FirstInitSize;

		PrintDbgMessage<bDoDebugPrinting>("{} --> NameInfos.size(): 0x{:X} -> 0x{:X}", __FUNCTION__, FirstInitSize, PackageManager::PackageInfos.size());
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInfo()
	{
		PackageManager::Init();

		UEObject EnginePackage = ObjectArray::FindObjectFast("Engine", EClassCastFlags::Package);
		UEObject CoreUObjectPackage = ObjectArray::FindObjectFast("CoreUObject", EClassCastFlags::Package);

		PackageInfoHandle Engine = PackageManager::GetInfo(EnginePackage);
		PackageInfoHandle CoreUObject = PackageManager::GetInfo(CoreUObjectPackage);

#define PackageInfoHandleToDebugInfo(InfoHandle) \
	InfoHandle.GetNameCollisionPair().first, \
	InfoHandle.GetNameCollisionPair().second, \
	InfoHandle.HasClasses(), \
	InfoHandle.HasStructs(), \
	InfoHandle.HasFunctions(), \
	InfoHandle.HasParameterStructs(), \
	InfoHandle.HasEnums(), \
	InfoHandle.GetSortedStructs().GetNumEntries(), \
	InfoHandle.GetSortedClasses().GetNumEntries(), \
	InfoHandle.GetFunctions().size(), \
	InfoHandle.GetEnums().size(), \
	InfoHandle.GetPackageDependencies().StructsIterationHitCount, \
	InfoHandle.GetPackageDependencies().ClassesIterationHitCount, \
	InfoHandle.GetPackageDependencies().StructsDependencies.size(), \
	InfoHandle.GetPackageDependencies().ClassesDependencies.size(), \
	InfoHandle.GetPackageDependencies().ParametersDependencies.size() \

		constexpr const char* FmtString = R"(
{}[{}]:
{{
	bHasEnums = {},
	bHasStructs = {},
	bHasClasses  = {},
	bHasFunctions = {},
	bHasParams = {},
	StructsSorted.size() = 0x{:X},
	ClassesSorted.size() = 0x{:X},
	Functions.size() = 0x{:X},
	Enums.size() = 0x{:X},
	{{
		StructsIterationHitCount = {},
		ClassesIterationHitCount = {},
		StructsDependencies.size() = 0x{:X},
		ClassesDependencies.size() = 0x{:X},
		ParametersDependencies.size() = 0x{:X},
	}}
}};
)";


		PrintDbgMessage<bDoDebugPrinting>(FmtString, PackageInfoHandleToDebugInfo(Engine));
		PrintDbgMessage<bDoDebugPrinting>(FmtString, PackageInfoHandleToDebugInfo(CoreUObject));
		PrintDbgMessage<bDoDebugPrinting>("");

		bool bSuccededTestWithoutError = true;

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestIncludeTypes()
	{
		bool bSuccededTestWithoutError = true;

		for (auto [PackageIndex, Info] : PackageManager::PackageInfos)
		{
			for (auto [AnotherPackageIndex, ReqInfo] : Info.PackageDependencies.StructsDependencies)
			{
				if (ReqInfo.bShouldIncludeClasses)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {}_structs.hpp rquires {}_classes.hpp\n", ObjectArray::GetByIndex(PackageIndex).GetName(), ObjectArray::GetByIndex(AnotherPackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}

				if (ReqInfo.PackageIdx == PackageIndex && ReqInfo.bShouldIncludeStructs)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {} requires itself\n", ObjectArray::GetByIndex(PackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}
			}

			for (auto [AnotherPackageIndex, ReqInfo] : Info.PackageDependencies.ClassesDependencies)
			{
				if (ReqInfo.PackageIdx == PackageIndex && ReqInfo.bShouldIncludeClasses)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {} requires itself\n", ObjectArray::GetByIndex(PackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}
			}
		}
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestMutliIterations()
	{
		bool bSuccededTestWithoutError = true;

		PackageManager::Init();

		VisitedNodeContainerType Visited;

		std::vector<int32> Indices;
		std::vector<int32> SecondIndices;
		Indices.reserve(PackageManager::PackageInfos.size());

		bool bIsSecondIteration = false;

		PackageManager::FindCycleCallbackType OnElementVisit = [&](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void
		{
			if (bIsSecondIteration)
			{
				SecondIndices.push_back(OldParams.RequiredPackage);
				return;
			}

			Indices.push_back(OldParams.RequiredPackage);
		};

		PackageManager::IterateDependencies(OnElementVisit);
		bIsSecondIteration = true;
		PackageManager::IterateDependencies(OnElementVisit);

		bSuccededTestWithoutError = Indices == SecondIndices;

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestFindCyclidDependencies()
	{
		bool bSuccededTestWithoutError = true;

		PackageManager::Init();

		PackageManager::FindCycleCallbackType OnCycleFound = [&bSuccededTestWithoutError](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void
		{
			std::string PrevName = ObjectArray::GetByIndex(NewParams.PrevPackage).GetValidName() + (NewParams.bWasPrevNodeStructs ? "_structs" : "_classes");
			std::string CurrName = ObjectArray::GetByIndex(NewParams.RequiredPackage).GetValidName() + (bIsStruct ? "_structs" : "_classes");

			PrintDbgMessage<bDoDebugPrinting>("Cycle between: Current - '{}' and Previous - '{}'", CurrName, PrevName);
			bSuccededTestWithoutError = false;
		};

		PackageManager::FindCycle(OnCycleFound);

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestCyclicDependencyDetection()
	{
		//bool bSuccededTestWithoutError = false;
		//
		//PackageManager::Init();
		//
		//VisitedNodeContainerType Visited;
		//PackageManager::OverrideMaptType CyclicGraph;
		//
		//
		///*
        //                                          
        //           +--------------+               
        //           | CYCLIC GRAPH |               
        //           +--------------+               
        //                                          
        //                  +------------------+    
        //                  |                  |    
        //            +-----+-----+            |    
        //            | 0_structs |            |    
        //            +-----+-----+   CYCLE    |    
        //                  |                  |    
        //         +--------+--------+         |    
        //         |                 |         |    
        //   +-----+-----+     +-----+-----+   |    
        //   | 1_classes |     | 1_structs |---+    
        //   +-----------+     +-----------+        
        //                                          
		//*/
		//
		///* Setup cyclic graph as above */
		//CyclicGraph[0].PackageDependencies.StructsDependencies[1] = { .PackageIdx = 1, .bShouldInclude = {.Structs = true, .Classes = false} };
		//CyclicGraph[1].PackageDependencies.StructsDependencies[0] = { .PackageIdx = 0, .bShouldInclude = {.Structs = true, .Classes = false} };
		//CyclicGraph[1].PackageDependencies.ClassesDependencies[0] = { .PackageIdx = 0, .bShouldInclude = {.Structs = true, .Classes = false} };
		//
		//for (auto& [FakePackageIndex, Info] : CyclicGraph)
		//{
		//	Visited.clear();
		//	Params.Requriements = { FakePackageIndex, { true, true } };
		//
		//	const bool bFoundCycle = false; // FindCycle(Params, !bDoDebugPrinting, false);
		//
		//	if (bFoundCycle)
		//		bSuccededTestWithoutError = true;
		//}
		//
		//std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUniquePackageNameGeneration()
	{
		bool bSuccededTestWithoutError = true;

		PackageManager::Init();

		std::unordered_set<std::string> Names;

		for (auto [PackageIndex, Info] : PackageManager::PackageInfos)
		{
			const auto [It, bWasInserted] = Names.insert(PackageInfoHandle(Info).GetName());
			if (!bWasInserted)
			{
				bSuccededTestWithoutError = false;
				PrintDbgMessage<bDoDebugPrinting>("PackageManagerTest: Name '{} is duplicate!'", PackageInfoHandle(Info).GetName());
			}
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};
