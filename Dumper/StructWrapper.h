#pragma once
#include "ObjectArray.h"
#include "NameCollisionHandler.h"
#include "StructManager.h"
#include "MemberManager.h"
#include "PredefinedMembers.h"

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
        const PredefinedStruct* PredefStruct;
    };

    StructInfoHandle InfoHandle;

    bool bIsUnrealStruct = false;

public:
    StructWrapper(const PredefinedStruct* const Predef);

    StructWrapper(const UEStruct Str);

public:
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

    bool IsClass() const;
    bool IsFunction() const;

    bool IsValid() const;

    MemberManager GetMembers() const;
};
