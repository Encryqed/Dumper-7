#pragma once

#include "Types.h"
#include "ObjectArray.h"
#include <unordered_set>

struct PackageDependencyManager
{
	friend class Package;

	union DependencyInfo
	{
		bool bWasProcessed;

		struct
		{

		};
	};

	// int32 - PackageIndex
	// bool - bWasIncluded
	//		DepdendencyInfo - PackageFiles required by this packages
	//std::unordered_map<int32, std::pair<DependencyInfo, std::unordered_set<int32>>> AllDependencies;
	std::unordered_map<int32, std::pair<bool, std::unordered_map<int32, DependencyFlags>>> AllDependencies;

	PackageDependencyManager() = default;

	PackageDependencyManager(int32 PackageIdx)
	{
		if (AllDependencies.find(PackageIdx) == AllDependencies.end())
			AllDependencies[PackageIdx] = { false, { }};
	}

	PackageDependencyManager(int32 PackageIdx, std::unordered_map<int32, DependencyFlags>& Dependencies)
	{
		AllDependencies[PackageIdx].second = Dependencies;
	}

	void RemoveDependant(const int32 PackageIndex)
	{
		AllDependencies.erase(PackageIndex);
	}

	inline void AddDependency(const int32 DepandantIdx, const int32 DependencyIndex, const DependencyFlags Info = None)
	{
		//the following line hurts me, a lot!
		(int&)AllDependencies[DepandantIdx].second[DependencyIndex] |= (int)Info;
	}

	/* Only use this when sorting struct dependencies */
	void GenerateStructSorted(class Package& Pack, int32 StructIdx);

	/* Only use this when sorting class dependencies */
	void GenerateClassSorted(class Package& Pack, int32 ClassIdx);

	/* Only use this when sorting package dependencies */
	void GetIncludesForPackage(int32 PackageIdx, std::string& OutRef, DependencyFlags Info = NeedsAllFiles);

	static void GetObjectDependency(UEObject Obj, std::unordered_set<int32>& Store);
};

class Package
{
	friend PackageDependencyManager;

public:
	static PackageDependencyManager PackageSorter;

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

	void GatherDependencies(std::vector<int32_t>& PackageMembers);

	void Process(std::vector<int32_t>& PackageMembers);

	void GenerateMembers(std::vector<UEProperty>& MemberVector, UEStruct& Super, Types::Struct& Struct);
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
