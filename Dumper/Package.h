#pragma once

#include <unordered_set>
#include <filesystem>

#include "Types.h"
#include "ObjectArray.h"

namespace fs = std::filesystem;

enum class EIncludeFileType
{
	Struct,
	Class,
	Params
};

struct PackageDependencyManager
{
	friend class Package;

	//int32 = PackageIdx
	//pair<bool, set<int32>> = pair<bIsIncluded, set<DependencyIndices>>
	std::unordered_map<int32, std::pair<bool, std::unordered_set<int32>>> AllDependencies;

	PackageDependencyManager() = default;

	PackageDependencyManager(int32 PackageIdx)
	{
		if (AllDependencies.find(PackageIdx) == AllDependencies.end())
			AllDependencies[PackageIdx] = { { false }, { } };
	}

	void RemoveDependant(const int32 PackageIndex)
	{
		AllDependencies.erase(PackageIndex);
	}

	inline void AddPackage(const int32 PackageIdx)
	{
		if (AllDependencies.find(PackageIdx) == AllDependencies.end())
		{
			AllDependencies[PackageIdx] = { { false }, { } };
		}
	}

	inline void AddDependency(const int32 DepandantIdx, const int32 DependencyIndex)
	{
		AllDependencies[DepandantIdx].second.insert(DependencyIndex);
	}

	/* Only use this when sorting struct dependencies */
	void GenerateStructSorted(class Package& Pack, const int32 StructIdx);

	/* Only use this when sorting class dependencies */
	void GenerateClassSorted(class Package& Pack, const int32 ClassIdx);

	/* Only use this when sorting package dependencies */
	void GetIncludesForPackage(const int32 Index, EIncludeFileType FileType, std::string& OutRef, bool bCommentOut);

	static void GetPropertyDependency(UEProperty Prop, std::unordered_set<int32>& Store);
	static void GetFunctionDependency(UEFunction Func, std::unordered_set<int32>& Store);
};

class Package
{
	friend PackageDependencyManager;

public:
	static std::ofstream DebugAssertionStream;
	static PackageDependencyManager PackageSorterClasses; // "PackageName_classes.hpp"
	static PackageDependencyManager PackageSorterStructs; // "PackageName_structs.hpp"
	static PackageDependencyManager PackageSorterParams; // "PackageName_parameters.hpp"

	PackageDependencyManager StructSorter;
	PackageDependencyManager ClassSorter;

private:
	UEObject PackageObject;

public:
	std::vector<Types::Enum> AllEnums;
	std::vector<Types::Struct> AllStructs;
	std::vector<Types::Class> AllClasses;
	std::vector<Types::Function> AllFunctions;

	Package(UEObject _Object)
		: PackageObject(_Object)
	{
	}

	static void InitAssertionStream(fs::path& GenPath);
	static void CloseAssertionStream();

	void GatherDependencies(std::vector<int32_t>& PackageMembers);

	void Process(std::vector<int32_t>& PackageMembers);

	void GenerateMembers(std::vector<UEProperty>& MemberVector, UEStruct& Super, Types::Struct& Struct, int32 StructSize, int32 SuperSize);
	Types::Function GenerateFunction(UEFunction& Function, UEStruct& Super);
	Types::Struct GenerateStruct(UEStruct& Struct, bool bIsFunction = false);
	Types::Class GenerateClass(UEClass& Class);
	Types::Enum GenerateEnum(UEEnum& Enum);

	Types::Member GenerateBytePadding(int32 Offset, int32 PadSize, std::string&& Reason);
	Types::Member GenerateBitPadding(int32 Offset, int32 PadSize, std::string&& Reason);

	inline bool IsEmpty()
	{
		return AllEnums.empty() && AllClasses.empty() && AllStructs.empty() && AllFunctions.empty();
	}

	inline UEObject DebugGetObject()
	{
		return PackageObject;
	}
};
