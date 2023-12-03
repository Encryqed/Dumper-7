#pragma once
#include <string>
#include <vector>

#include "Enums.h"

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

struct PredefinedFunction
{
    std::string RetType;
    std::string Name;

    std::vector<std::pair<std::string, std::string>> Params;

    bool bIsStatic;
};

struct PredefinedElements
{
    std::vector<PredefinedMember> Members;
    std::vector<PredefinedFunction> Functions;
};

struct PredefinedStruct
{
    std::string UniqueName;
    int32 Size;
    int32 Alignment;
    bool bUseExplictAlignment;
    bool bIsFinal;

    const PredefinedStruct* Super;

    std::vector<PredefinedMember> Properties;
    std::vector<PredefinedFunction> Functions;
};
