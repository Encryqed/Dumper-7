#pragma once

#include "UnrealObjects.h"
#include "DependencyManager.h"
#include "HashStringTable.h"

class PackageInfoHandle;

class PackageManager;

struct PackageCollisionInfo
{
private:
	friend class PackageManager;

private:
	HashStringTableIndex NameIdx;

	std::unordered_set<int32> PackageDependencies;

	DependencyManager Structs;
	DependencyManager Classes;
	std::vector<int32> Enums;
	std::vector<int32> Functions;

public:
	std::string GetUniqueName() const;
	uint64 GetValue() const;

	uint8 GetCollisionCount() const;
};

struct PackageInfo
{
private:
	friend class PackageInfoHandle;
	friend class PackageManager;

private:
	/* Name of this Package*/
	HashStringTableIndex Name;

	/* sizeof(UnderlayingType) */
	int32 UnderlyingTypeSize = 0x1;

	/* Infos on all members and if there are any collisions between member-names */
	std::vector<PackageCollisionInfo> MemberInfos;
};

struct CollisionInfoIterator
{
private:
	const std::vector<PackageCollisionInfo>& CollisionInfos;

public:
	CollisionInfoIterator(const std::vector<PackageCollisionInfo>& Infos)
		: CollisionInfos(Infos)
	{
	}

public:
	auto begin() const { return CollisionInfos.cbegin(); }
	auto end() const { return CollisionInfos.end(); }
};

class PackageInfoHandle
{
private:
	const PackageInfo* Info;

public:
	PackageInfoHandle() = default;
	PackageInfoHandle(const PackageInfo& InInfo);

public:
	int32 GetUnderlyingTypeSize() const;
	const StringEntry& GetName() const;

	CollisionInfoIterator GetMemberCollisionInfoIterator() const;
};


class PackageManager
{
private:
	friend class PackageCollisionInfo;
	friend class PackageInfoHandle;
	friend class PackageManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /* PackageIndex */, PackageInfo>;

public:
	/* NameTable containing names of all Packages as well as information on name-collisions */
	static inline HashStringTable UniquePackageNameTable;

	/* Map containing infos on all Packages. Implemented due to information missing in the Unreal's reflection system (PackageSize). */
	static inline OverrideMaptType PackageInfos;

	static inline bool bIsInitialized = false;

private:
	static void InitInternal();

public:
	static void Init();

private:
	static inline const StringEntry& GetPackageName(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name];
	}

public:
	static inline const OverrideMaptType& GetPackageInfos()
	{
		return PackageInfos;
	}

	static inline bool IsPackageNameUnique(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name].IsUnique();
	}

	static inline PackageInfoHandle GetInfo(const UEObject Package)
	{
		if (!Package)
			return {};

		return PackageInfos.at(Package.GetIndex());
	}
};
