#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"

struct MemberNode
{
	/* Name of the class-member*/
	HashStringTableIndex Name;

	/* CustomTypeName if existing, eg. if type is "class AActor*" this is "AActor" */
	HashStringTableIndex CustomTypeName;

	/* Namespace of CustomTypeName, eg if type is "class Engine::UDuplicate*" this is "Engine" */
	HashStringTableIndex CustomTypeNameNamespace;

	/* Name of this property's class. Used for UnknownProperties, eg. FMulticastDelegateProperty */
	HashStringTableIndex PropertyClassName;

	int32 Offset;
	int32 Size;
	int32 ArrayDim = 0x1;

	EObjectFlags ObjFlags = EObjectFlags::NoFlags;
	EPropertyFlags PropertyFlags = EPropertyFlags::None;
	EClassCastFlags CastFlags = EClassCastFlags::None;

	bool bIsEnumClass = false;
	bool bIsBitField = false;
	uint8 BitFieldIndex = 0x0;
	uint8 BitMask = 0xFF;

	/* Prefer using other members instead of directly accessing UnrealProperty */
	UEProperty UnrealProperty = nullptr;
};

struct ParamNode : public MemberNode
{
	bool bIsOutParam;
	bool bIsRefParam;
	bool bIsRetParam;
};

struct UniqueNameBase
{
	/* full name --> "Class Engine.PlayerController", can be used to retreive RawName --> "PlayerController" */
	HashStringTableIndex FullName;

	/* prefixed name --> eg. "Some#Class" -> "USome_Class" */
	HashStringTableIndex PrefixedName;

	/* name of package --> eg. "CoreUObject" */
	HashStringTableIndex PackageName;

	inline const StringEntry& GetFullNameEntry(const HashStringTable& Names) const { return Names.GetStringEntry(FullName); }
	inline const StringEntry& GetRawNameEntry(const HashStringTable& Names) const { return Names.GetStringEntry(FullName); }

	inline const StringEntry& GetPrefixedNameEntry(const HashStringTable& Names) const { return Names.GetStringEntry(PrefixedName); }

	inline const StringEntry& GetPackageNameEntry(const HashStringTable& Names) const { return Names.GetStringEntry(PackageName); }
};

struct EnumNode : public UniqueNameBase
{
	/* Prefer using other members instead of directly accessing UnrealEnum */
	UEEnum UnrealEnum;

	std::vector<std::pair<std::string, int32>> NameValuePairs;
	int32 UnderlayingTypeSize;
};

struct StructNodeBase : public UniqueNameBase
{
	int32 Size;
	int32 SuperSize;
};

struct FunctionNode : public StructNodeBase
{
	/* [nullptr for PredefinedFunctions] Prefer using other members instead of directly accessing UnrealFunction */
	UEFunction UnrealFunction;

	std::vector<ParamNode> Params;
	class ClassNode* OuterClass;
	EFunctionFlags FuncFlags;

	int RetValueIndex = -1;
	bool bIsNative = false;
};

struct StructNode : public StructNodeBase
{
	/* Prefer using other members instead of directly accessing UnrealStruct */
	UEStruct UnrealStruct;

	StructNode* Super;

	/* Field for all Properties or PredefinedMembers */
	std::vector<MemberNode> Members;

	/* Field for all UFunctions or PredefinedFunctions */
	std::vector<FunctionNode> Functions;
};
