#pragma once
#include <unordered_map>
#include <unordered_set>
#include "HashStringTable.h"
#include "UnrealObjects.h"
#include "ObjectArray.h" /* for debug print */

struct StructInfo
{
	HashStringTableIndex Name;

	/* Unaligned size of this struct */
	int32 Size = INT_MAX;

	/* Alignment of this struct for alignas(Alignment), might be implicit */
	int32 Alignment = 0x1;


	/* Whether alignment should be explicitly specified with 'alignas(Alignment)' */
	bool bUseExplicitAlignment;

	/* Wheter this class is ever inherited from. Set to false when this struct is found to be another structs super */
	bool bIsFinal = true;

	/* Whether this struct is in a package that has cyclic dependencies. Actual index of cyclic package is stored in StructManager::CyclicStructsAndPackages */
	bool bIsPartOfCyclicPackage;
};

class StructManager;

class StructInfoHandle
{
private:
	const StructInfo* Info;

public:
	StructInfoHandle() = default;
	StructInfoHandle(const StructInfo& InInfo);

public:
	int32 GetSize() const;
	int32 GetUnalignedSize() const;
	int32 GetAlignment() const;
	bool ShouldUseExplicitAlignment() const;
	const StringEntry& GetName() const;
	bool IsFinal() const;

	bool IsPartOfCyclicPackage() const;
};

class StructManager
{
private:
	friend StructInfoHandle;
	friend class StructManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /*StructIdx*/, StructInfo>;
	using CycleInfoListType = std::unordered_map<int32 /*StructIdx*/, std::unordered_set<int32 /* Packages cyclic with this structs' package */>>;

private:
	/* NameTable containing names of all structs/classes as well as information on name-collisions */
	static inline HashStringTable UniqueNameTable;

	/* Map containing infos on all structs/classes. Implemented due to bugs/inconsistencies in Unreal's reflection system */
	static inline OverrideMaptType StructInfoOverrides;

	/* Map containing infos on all structs/classes that are within a packages that has cyclic dependencies */
	static inline CycleInfoListType CyclicStructsAndPackages;

	static inline bool bIsInitialized = false;

private:
	static void InitAlignmentsAndNames();
	static void InitSizesAndIsFinal();

public:
	static void Init();

private:
	static inline const StringEntry& GetName(const StructInfo& Info)
	{
		return UniqueNameTable[Info.Name];
	}

public:
	static inline const OverrideMaptType& GetStructInfos()
	{
		return StructInfoOverrides;
	}

	static inline bool IsStructNameUnique(HashStringTableIndex NameIndex)
	{
		return UniqueNameTable[NameIndex].IsUnique();
	}

	// debug function
	static inline std::string GetName(HashStringTableIndex NameIndex)
	{
		return UniqueNameTable[NameIndex].GetName();
	}

	static inline StructInfoHandle GetInfo(const UEStruct Struct)
	{
		if (!Struct)
			return {};

		return StructInfoOverrides.at(Struct.GetIndex());
	}

	static inline bool IsStructCyclicWithPackage(int32 StructIndex, int32 PackageIndex)
	{
		auto It = CyclicStructsAndPackages.find(StructIndex);
		if (It != CyclicStructsAndPackages.end())
			return It->second.contains(PackageIndex);

		return false;
	}

	/* 
	* Utility function for PackageManager::PostInit to handle the initialization of our list of cyclic structs and their respective packages
	* 
	* Marks StructInfo as 'bIsPartOfCyclicPackage = true' and adds struct to 'CyclicStructsAndPackages'
	*/
	static inline void PackageManagerSetCycleForStruct(int32 StructIndex, int32 PackageIndex)
	{
		std::cout << "Set struct as cyclic: '" << ObjectArray::GetByIndex(StructIndex).GetFullName() << "'\n";

		StructInfo& Info = StructInfoOverrides.at(StructIndex);

		Info.bIsPartOfCyclicPackage = true;


		CyclicStructsAndPackages[StructIndex].insert(PackageIndex);
	}
};

