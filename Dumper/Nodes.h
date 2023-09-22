#pragma once
#include "ObjectArray.h"

struct MemberNode
{
	std::string Type;
	std::string Name;

	int32 Offset;
	int32 Size;
	int32 ArrayDim;
	EObjectFlags ObjFlags;
	EPropertyFlags PropFlags;

	/* Prefer using other members instead of directly accessing UnrealProperty */
	UEProperty* UnrealProperty;
};

struct ParamNode : public MemberNode
{
	bool bIsOutParam;
	bool bIsRefParam;
	bool bIsRetParam;
};

struct UniqueNameBase
{
	/* "unedited" name --> eg. "PlayerController", "Vector", "ENetRole" */
	std::string RawName;

	/* unique name --> eg. "APlayerController", "PackageName::UDuplicatedClass", "ENetRol" */
	std::string UniqueName;

	/* full name --> "Class Engine.PlayerController" */
	std::string FullName;
};

struct EnumNode : public UniqueNameBase
{
	/* Prefer using other members instead of directly accessing UnrealEnum */
	UEEnum* UnrealEnum;

	std::vector<std::pair<std::string, int32>> NameValuePairs;
	int32 UnderlayingTypeSize;
};

struct StructNodeBase : public UniqueNameBase
{
	int32 Size;
};

struct StructNode : public StructNodeBase
{
	/* Prefer using other members instead of directly accessing UnrealStruct */
	UEStruct* UnrealStruct;

	StructNode* Super;

	/* Field for all Properties or PredefinedMembers */
	std::vector<MemberNode> Members;

	/* Field for all UFunctions or PredefinedFunctions */
	std::vector<FunctionNode> Functions;
};

struct FunctionNode : public StructNodeBase
{
	/* [nullptr for PredefinedFunctions] Prefer using other members instead of directly accessing UnrealFunction */
	UEFunction* UnrealFunction;

	std::vector<ParamNode> Params;
	class ClassNode* OuterClass;
	EFunctionFlags FuncFlags;

	int RetValueIndex = -1;
	bool bIsNative = false;
};