#pragma once

#include "CollisionManager.h"


class EnumInfoHandle;

class EnumManager;

struct EnumCollisionInfo
{
private:
	friend class EnumManager;

private:
	HashStringTableIndex MemberName;
	uint64 MemberValue;

	uint8 CollisionCount = 0;

public:
	std::string GetUniqueName() const;
	std::string GetRawName() const;
	uint64 GetValue() const;

	uint8 GetCollisionCount() const;
};

struct EnumInfo
{
private:
	friend class EnumInfoHandle;
	friend class EnumManager;

private:
	/* Name of this Enum*/
	HashStringTableIndex Name;

	/* sizeof(UnderlyingType) */
	uint8 UnderlyingTypeSize = 0x1;

	/* Whether any member holds a negative value, in which case the underlying type must be signed */
	bool bIsSigned = false;

	/* Wether an occurence of this enum was found, if not guess the type by the enums' max value */
	bool bWasInstanceFound = false;

	/* Whether this enums' size was initialized before */
	bool bWasEnumSizeInitialized = false;

	/* Infos on all members and if there are any collisions between member-names */
	std::vector<EnumCollisionInfo> MemberInfos;
};

struct CollisionInfoIterator
{
private:
	const std::vector<EnumCollisionInfo>& CollisionInfos;

public:
	CollisionInfoIterator(const std::vector<EnumCollisionInfo>& Infos)
		: CollisionInfos(Infos)
	{
	}

public:
	auto begin() const { return CollisionInfos.cbegin(); }
	auto end() const { return CollisionInfos.end(); }
};

class EnumInfoHandle
{
private:
	const EnumInfo* Info = nullptr;

public:
	EnumInfoHandle() = default;
	EnumInfoHandle(const EnumInfo& InInfo);

public:
	/* False for handles to enums that were never registered (e.g. an EnumProperty without a valid underlaying property) */
	bool IsValid() const { return Info != nullptr; }

	uint8 GetUnderlyingTypeSize() const;
	bool IsUnderlyingTypeSigned() const;
	const StringEntry& GetName() const;

	int32 GetNumMembers() const;

	CollisionInfoIterator GetMemberCollisionInfoIterator() const;
};


class EnumManager
{
private:
	friend struct EnumCollisionInfo;
	friend class EnumInfoHandle;

public:
	using OverrideMaptType = std::unordered_map<int32 /* EnumIndex */, EnumInfo>;
	using IllegalNameContaierType = std::vector<HashStringTableIndex>;

private:
	/* NameTable containing names of all enums as well as information on name-collisions */
	static inline HashStringTable UniqueEnumNameTable;

	/* Map containing infos on all enums. Implemented due to information missing in the Unreal's reflection system (EnumSize). */
	static inline OverrideMaptType EnumInfoOverrides;

	/* NameTable containing names of all enum-values as well as information on name-collisions */
	static inline HashStringTable UniqueEnumValueNames;

	/* List containing names-indices which contain illegal enum names such as 'PF_MAX' */
	static inline IllegalNameContaierType IllegalNames;

	static inline bool bIsInitialized = false;

private:
	static void InitInternal();
	static void InitIllegalNames();

public:
	static void Init();

private:
	static inline const StringEntry& GetEnumName(const EnumInfo& Info)
	{
		return UniqueEnumNameTable[Info.Name];
	}

	static inline const StringEntry& GetValueName(const EnumCollisionInfo& Info)
	{
		return UniqueEnumValueNames[Info.MemberName];
	}

public:
	static inline const OverrideMaptType& GetEnumInfos()
	{
		return EnumInfoOverrides;
	}

	static inline bool IsEnumNameUnique(const EnumInfo& Info)
	{
		return UniqueEnumNameTable[Info.Name].IsUnique();
	}

	static inline EnumInfoHandle GetInfo(const UEEnum Enum)
	{
		if (!Enum)
			return {};

		/* Use find rather than at() - a property can reference an enum that was never registered (returns an invalid handle instead of throwing) */
		auto It = EnumInfoOverrides.find(Enum.GetIndex());
		if (It == EnumInfoOverrides.end())
			return {};

		return It->second;
	}
};

