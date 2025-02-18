#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "Unreal/Enums.h"
#include "Unreal/UnrealObjects.h"

struct PredefinedMember
{
    std::string Comment;

    std::string Type;
    std::string Name;

    int32 Offset;
    int32 Size;
    int32 ArrayDim;
    int32 Alignment;

    bool bIsStatic;
    bool bIsZeroSizeMember;

    bool bIsBitField;
    uint8 BitIndex;
    uint8 BitCount = 0x1;

    std::string DefaultValue = std::string();
};

struct PredefinedFunction
{
    std::string CustomComment;
    std::string CustomTemplateText = std::string();
    std::string ReturnType;
    std::string NameWithParams;
    std::string NameWithParamsWithoutDefaults = std::string();

    std::string Body;

    bool bIsStatic;
    bool bIsConst;
    bool bIsBodyInline;
};

struct PredefinedElements
{
    std::vector<PredefinedMember> Members;
    std::vector<PredefinedFunction> Functions;
};

struct PredefinedStruct
{
    std::string CustomTemplateText = std::string();
    std::string UniqueName;
    int32 Size;
    int32 Alignment;
    bool bUseExplictAlignment;
    bool bIsFinal;
    bool bIsClass;
    bool bIsUnion;

    const PredefinedStruct* Super;

    std::vector<PredefinedMember> Properties;
    std::vector<PredefinedFunction> Functions;
};

/* unordered_map<StructIndex, Members/Functions> */
using PredefinedMemberLookupMapType = std::unordered_map<int32 /* StructIndex */, PredefinedElements /* Members/Functions */>;

// requires strict weak ordering
inline bool CompareUnrealProperties(UEProperty Left, UEProperty Right)
{
    if (Left.IsA(EClassCastFlags::BoolProperty) && Right.IsA(EClassCastFlags::BoolProperty))
    {
        if (Left.GetOffset() == Right.GetOffset())
        {
            return Left.Cast<UEBoolProperty>().GetFieldMask() < Right.Cast<UEBoolProperty>().GetFieldMask();
        }
    }

    return Left.GetOffset() < Right.GetOffset();
};

// requires strict weak ordering
inline bool ComparePredefinedMembers(const PredefinedMember& Left, const PredefinedMember& Right)
{
    // if both members are static, sort lexically
    if (Left.bIsStatic && Right.bIsStatic)
        return Left.Name < Right.Name;

    // if one member is static, return true if Left is static, false if Right
    if (Left.bIsStatic || Right.bIsStatic)
        return Left.bIsStatic > Right.bIsStatic;

    return Left.Offset < Right.Offset;
};

/*
Order:
    static non-inline
    non-inline
    static inline
    inline
*/

// requires strict weak ordering
inline bool CompareUnrealFunctions(UEFunction Left, UEFunction Right)
{
    const bool bIsLeftStatic = Left.HasFlags(EFunctionFlags::Static);
    const bool bIsRightStatic = Right.HasFlags(EFunctionFlags::Static);

    const bool bIsLeftConst = Left.HasFlags(EFunctionFlags::Const);
    const bool bIsRightConst = Right.HasFlags(EFunctionFlags::Const);

    // Static members come first
    if (bIsLeftStatic != bIsRightStatic)
        return bIsLeftStatic > bIsRightStatic;

    // Const members come last
    if (bIsLeftConst != bIsRightConst)
        return bIsLeftConst < bIsRightConst;

    return Left.GetIndex() < Right.GetIndex();
};

// requires strict weak ordering
inline bool ComparePredefinedFunctions(const PredefinedFunction& Left, const PredefinedFunction& Right)
{
    // Non-inline members come first
    if (Left.bIsBodyInline != Right.bIsBodyInline)
        return Left.bIsBodyInline < Right.bIsBodyInline;

    // Static members come first
    if (Left.bIsStatic != Right.bIsStatic)
        return Left.bIsStatic > Right.bIsStatic;

    // Const members come last
    if (Left.bIsConst != Right.bIsConst)
        return Left.bIsConst < Right.bIsConst;

    return Left.NameWithParams < Right.NameWithParams;
};


