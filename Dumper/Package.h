#pragma once

#include <unordered_set>
#include <filesystem>

#include "Types.h"
#include "ObjectArray.h"

namespace fs = std::filesystem;

struct PackageDependencyManager
{
	friend class Package;

	struct DependencyInfo
	{
		int32 Index;
		
		//PackageSorting vars
		mutable bool bStructFileNeeded;
		mutable bool bClassFileNeeded;

		DependencyInfo() = default;

		DependencyInfo(int32 Idx)
			: Index(Idx), bStructFileNeeded(false), bClassFileNeeded(false)
		{
		}

		DependencyInfo(int32 Idx, bool bNeedsStructFile, bool bNeedsClassFile)
			: Index(Idx), bStructFileNeeded(bNeedsStructFile), bClassFileNeeded(bNeedsClassFile)
		{
		}

		const DependencyInfo& operator=(const DependencyInfo& Other) const
		{
			bStructFileNeeded = bStructFileNeeded || Other.bStructFileNeeded;
			bClassFileNeeded = bClassFileNeeded || Other.bClassFileNeeded;

			return *this;
		}

		bool operator==(const DependencyInfo& Other) const
		{
			return Index == Other.Index;
		}
	};

	union IncludeStatus
	{
		bool bIsIncluded;

		struct
		{
			bool bIsStructFileIncluded;
			bool bIsClassFileIncluded;
		};
	};
	
	struct DependencyInfoHasher
	{
		size_t operator()(const DependencyInfo& R) const
		{
			return R.Index;
		}
	};

	// int32 - PackageIndex
	// bool - bWasIncluded
	//		DepdendencyInfo - PackageFiles required by this packages
	//std::unordered_map<int32, std::pair<DependencyInfo, std::unordered_set<int32>>> AllDependencies;
	std::unordered_map<int32, std::pair<IncludeStatus, std::unordered_set<DependencyInfo, DependencyInfoHasher>>> AllDependencies;

	PackageDependencyManager() = default;

	PackageDependencyManager(int32 PackageIdx)
	{
		if (AllDependencies.find(PackageIdx) == AllDependencies.end())
			AllDependencies[PackageIdx] = { { false }, { } };
	}

	PackageDependencyManager(int32 PackageIdx, std::unordered_set<DependencyInfo, DependencyInfoHasher>& Dependencies)
	{
		AllDependencies[PackageIdx].second = Dependencies;
	}

	void RemoveDependant(const int32 PackageIndex)
	{
		AllDependencies.erase(PackageIndex);
	}

	inline void AddDependency(const int32 DepandantIdx, const int32 DependencyIndex)
	{
		AllDependencies[DepandantIdx].second.insert(DependencyIndex);
	}

	inline void AddDependency(const int32 DepandantIdx, const DependencyInfo& Info)
	{
		auto IteratorBoolPair = AllDependencies[DepandantIdx].second.insert(Info);

		if (!IteratorBoolPair.second)
		{
			*IteratorBoolPair.first = Info;
		}
	}

	/* Only use this when sorting struct dependencies */
	void GenerateStructSorted(class Package& Pack, int32 StructIdx);

	/* Only use this when sorting class dependencies */
	void GenerateClassSorted(class Package& Pack, int32 ClassIdx);

	/* Only use this when sorting package dependencies */
	void GetIncludesForPackage(const DependencyInfo& Info, std::string& OutRef);

	static void GetObjectDependency(UEObject Obj, std::unordered_set<int32>& Store);
};

class Package
{
	friend PackageDependencyManager;

public:
	static std::ofstream DebugAssertionStream;
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
