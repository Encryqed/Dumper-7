#pragma once
#include "CollisionManager.h"

struct EnumInfo
{
	HashStringTableIndex Name;

	int32 UnderlyingTypeSize = INT_MAX;
};

class EnumManager;

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
};


class EnumManager
{
private:
	friend EnumInfoHandle;
	friend class EnumManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /* EnumIndex */, EnumInfo>;

public:
	/* CollisionManager containing information on enum-member */
	static CollisionManager EnumNames;

	/* NameTable containing names of all enums as well as information on name-collisions */
	static inline HashStringTable UniqueNameTable;

	/* Map containing infos on all enums. Implemented due to information missing in the Unreal's reflection system (EnumSize). */
	static inline OverrideMaptType EnumInfoOverrides;

	static inline bool bIsInitialized = false;

private:
	static void InitUnderlayingSizesAndName();

public:
	static void Init();

private:
	static inline const StringEntry& GetName(const EnumInfo& Info)
	{
		return UniqueNameTable[Info.Name];
	}

public:
	static inline const OverrideMaptType& GetEnumInfos()
	{
		return EnumInfoOverrides;
	}

	static inline bool IsEnumNameUnique(HashStringTableIndex NameIndex)
	{
		return UniqueNameTable[NameIndex].IsUnique();
	}

	// debug function
	static inline std::string GetName(HashStringTableIndex NameIndex)
	{
		return UniqueNameTable[NameIndex].GetName();
	}

	static inline EnumInfoHandle GetInfo(const UEEnum Enum)
	{
		if (!Enum)
			return {};

		return EnumInfoOverrides.at(Enum.GetIndex());
	}
};