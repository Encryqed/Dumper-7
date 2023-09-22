#pragma once
#include "GeneratorBase.h"

class CppGenerator : public GeneratorBase
{
    virtual void GenerateStruct(const StructInfo& Struct) override;
    virtual void GenerateClass(const ClassInfo& Class) override;
    virtual void GenerateFunction(const FunctionInfo& Function) override;

    virtual void GenerateSDK() override;

    virtual void InitPredefinedMembers() override;
    virtual void InitPredefinedFunctions() override;
};