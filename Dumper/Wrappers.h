#pragma once
#include "ObjectArray.h"
#include "StructManager.h"

struct PredefinedStruct
{
    std::string UniqueName;
    int32 Size;
    int32 Alignment;
    bool bUseExplictAlignment;
    bool bIsFinal;

    const PredefinedStruct* Super;
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
    union
    {
        const UEStruct Struct;
        const PredefinedStruct* PredefStruct;
    };
    StructInfoHandle InfoHandle;

    bool bIsUnrealStruct = false;

public:
    StructWrapper(const PredefinedStruct* Predef);

    StructWrapper(UEStruct Str);

    //StructWrapper(const StructWrapper&) = default;

public:
    UEStruct GetStruct() const;

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