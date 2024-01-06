#pragma once
#include "HashStringTable.h"
#include "PackageManager.h"
#include "TestBase.h"

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
		TestFindCyclidDependencies<bDoDebugPrinting>();
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
	InfoHandle.GetName().GetName(), \
	InfoHandle.GetName().IsUnique(), \
	InfoHandle.HasClasses(), \
	InfoHandle.HasStructs(), \
	InfoHandle.HasFunctions(), \
	InfoHandle.HasParameterStructs(), \
	InfoHandle.HasEnums(), \
	InfoHandle.GetSortedStructs().GetNumEntries(), \
	InfoHandle.GetSortedClasses().GetNumEntries(), \
	InfoHandle.GetFunctions().size(), \
	InfoHandle.GetEnums().size(), \
	InfoHandle.GetPackageDependencies().bIsIncluded.Structs, \
	InfoHandle.GetPackageDependencies().bIsIncluded.Classes, \
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
		{{
			bIsIncluded.Structs = {},
			bIsIncluded.Classes = {},
		}},
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
				if (ReqInfo.bShouldInclude.Classes)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {}_structs.hpp rquires {}_classes.hpp\n", ObjectArray::GetByIndex(PackageIndex).GetName(), ObjectArray::GetByIndex(AnotherPackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}

				if (ReqInfo.PackageIdx == PackageIndex)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {} itself\n", ObjectArray::GetByIndex(PackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}
			}

			for (auto [AnotherPackageIndex, ReqInfo] : Info.PackageDependencies.ClassesDependencies)
			{
				if (ReqInfo.PackageIdx == PackageIndex)
				{
					PrintDbgMessage<bDoDebugPrinting>("Error: {} itself\n", ObjectArray::GetByIndex(PackageIndex).GetName());
					bSuccededTestWithoutError = false;
				}
			}
		}
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestFindCyclidDependencies()
	{
		PackageManager::Init();

		VisitedNodeContainerType Visited;

		FindCycleParams Params = {
			.Nodes = PackageManager::PackageInfos,
			.PrevNode = -1,
			.Requriements = { 0, { true, true }},
			.VisitedNodes = Visited
		};

		for (auto [PackageIndex, Info] : PackageManager::PackageInfos)
		{
			Visited.clear();
			Params.Requriements.PackageIdx = PackageIndex;
			FindCycle(Params);
		}

		bool bSuccededTestWithoutError = true;

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};