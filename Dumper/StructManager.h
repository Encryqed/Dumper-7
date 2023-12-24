#pragma once
#include <unordered_map>
#include "HashStringTable.h"
#include "UnrealObjects.h"


/*
struct Struct1
{
	int a;
	int b;
	int c;
	uint8 Pad[0x10];
	int d;
	int e;
	int f;
};

Struct1_PredefinedMembers: {
	"void*", "Something", 0x10, 0x8
}

Result:
struct Struct1
{
public:
	int32                 a;                              // 0x0000(0x0004)
	int32                 b;                              // 0x0004(0x0004)
	int32                 c;                              // 0x0008(0x0004)
	uint8                 Pad_0[0x4];                     // 0x000C(0x0004)
	void*                 Something;                      // 0x0010(0x0008)
	uint8                 Pad_1[0x4];                     // 0x0018(0x0004)
	int32                 e;                              // 0x001C(0x0004)
	int32                 f;                              // 0x0020(0x0004)
};
*/

struct StructInfo
{
	HashStringTableIndex Name;

	int32 Size = INT_MAX;
	int32 Alignment = 0x1;

	bool bUseExplicitAlignment; // whether alignment should be specified with 'alignas(Alignment)'
	bool bIsFinal = true; // wheter this class is ever inherited from. set to false when this struct is found to be another structs super
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
	int32 GetAlignment() const;
	bool ShouldUseExplicitAlignment() const;
	const StringEntry& GetName() const;
	bool IsFinal() const;
};

class StructManager
{
private:
	friend StructInfoHandle;
	friend class StructManagerTest;

public:
	using OverrideMaptType = std::unordered_map<int32 /*StructIdx*/, StructInfo>;

private:
	static inline HashStringTable UniqueNameTable;
	static inline OverrideMaptType StructInfoOverrides;

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

	static inline StructInfoHandle GetInfo(const UEStruct Struct)
	{
		if (!Struct)
			return {};

		//if (Struct.IsA(EClassCastFlags::Function))
		//	return {};

		return StructInfoOverrides.at(Struct.GetIndex());
	}
};

