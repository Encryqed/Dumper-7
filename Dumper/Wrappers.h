#pragma once
#include "ObjectArray.h"
#include "NameCollisionHandler.h"
#include "StructManager.h"

struct PredefinedMember
{
    std::string Type;
    std::string Name;

    int32 Offset;
    int32 Size;
    int32 ArrayDim;
    int32 Alignment;

    bool bIsStatic;
};

struct PredefinedStructBase
{
    std::string UniqueName;
    int32 Size;
    int32 Alignment;
    bool bUseExplictAlignment;
    bool bIsFinal;

    const PredefinedStructBase* Super;

    std::vector<PredefinedMember> Properties;
};

struct PredefinedFunction : public PredefinedStructBase
{
    std::string RetType;

    std::vector<std::pair<std::string, std::string>> Params;

    bool bIsStatic;
};

struct PredefinedStruct : public PredefinedStructBase
{
    std::vector<PredefinedFunction> Functions;
};

inline PredefinedStruct Test = {
    "UStruct",
    0x560,
    0x8,
    false,
    false
};

class StructWrapper
{
private:
    friend class PropertyWrapper;
    friend class FunctionWrapper;

private:
    union
    {
        const UEStruct Struct;
        const PredefinedStructBase* PredefStruct;
    };

    StructInfoHandle InfoHandle;

    bool bIsUnrealStruct = false;

public:
    StructWrapper(const PredefinedStructBase* const Predef);

    StructWrapper(const UEStruct Str);

private:
    UEStruct GetUnrealStruct() const;

public:
    std::string GetName() const;
    std::string GetFullName() const;
    StructWrapper GetSuper() const;

    /* Name, bIsUnique */
    std::pair<std::string, bool> GetUniqueName() const;
    int32 GetAlignment() const;
    int32 GetSize() const;
    bool ShouldUseExplicitAlignment() const;
    bool IsFinal() const;

    bool IsValid() const;
};

class PropertyWrapper
{
private:
    union
    {
        const UEProperty Property;
        const PredefinedMember* PredefProperty;
    };

    const StructWrapper* Struct;

    NameInfo Name;

    bool bIsUnrealProperty = false;

public:
    PropertyWrapper(const StructWrapper& Str, const PredefinedMember* Predef);

    PropertyWrapper(const StructWrapper& Str, UEProperty Prop);

public:
    std::string GetName() const;

    NameInfo GetNameCollisionInfo() const;

    int32 GetArrayDim() const;
    int32 GetSize() const;
    int32 GetOffset() const;
    EPropertyFlags GetPropertyFlags() const;

    bool IsReturnParam() const;

    std::string StringifyFlags() const;
};

class FunctionWrapper
{
private:
    union
    {
        const UEFunction Function;
        const PredefinedFunction* PredefFunction;
    };

    const StructWrapper* Struct;

    NameInfo Name;

    bool bIsUnrealFunction = false;

public:
    FunctionWrapper(const StructWrapper& Str, const PredefinedFunction* Predef);

    FunctionWrapper(const StructWrapper& Str, UEFunction Func);

public:
    StructWrapper AsStruct() const;

    std::string GetName() const;

    NameInfo GetNameCollisionInfo() const;

    PropertyWrapper GetReturnProperty() const;
    EFunctionFlags GetFunctionFlags() const;

    std::string StringifyFlags() const;
    std::string GetParamStructName() const;
};

