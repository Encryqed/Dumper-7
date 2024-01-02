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

	/* sizeof(UnderlayingType) */
	int32 UnderlyingTypeSize = 0x1;

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
	const EnumInfo* Info;

public:
	EnumInfoHandle() = default;
	EnumInfoHandle(const EnumInfo& InInfo);

public:
	int32 GetUnderlyingTypeSize() const;
	const StringEntry& GetName() const;

	CollisionInfoIterator GetMemberCollisionInfoIterator() const;
};


class EnumManager
{
private:
	friend class EnumCollisionInfo;
	friend class EnumInfoHandle;
	friend class EnumManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /* EnumIndex */, EnumInfo>;

public:
	/* NameTable containing names of all enums as well as information on name-collisions */
	static inline HashStringTable UniqueEnumNameTable;

	/* Map containing infos on all enums. Implemented due to information missing in the Unreal's reflection system (EnumSize). */
	static inline OverrideMaptType EnumInfoOverrides;

	/* NameTable containing names of all enum-values as well as information on name-collisions */
	static inline HashStringTable UniqueEnumValueNames;

	static inline bool bIsInitialized = false;

private:
	static void InitInternal();

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

		return EnumInfoOverrides.at(Enum.GetIndex());
	}
};

