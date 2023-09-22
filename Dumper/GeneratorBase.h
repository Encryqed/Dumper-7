#pragma once
#include "ObjectArray.h"
#include <filesystem>

namespace fs = std::filesystem;

struct StructInfo {};
struct ClassInfo {};
struct FunctionInfo {};

class GeneratorBase
{
protected:
    std::vector<int> Packages;

public:
    static void Init();

protected:
    virtual void SetupFolders(const std::string& FolderName, fs::path& OutFolder);
    virtual void SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder);

    virtual void GenerateStruct(const StructInfo& Struct) = 0;
    virtual void GenerateClass(const ClassInfo& Class) = 0;
    virtual void GenerateFunction(const FunctionInfo& Function) = 0;

public:
    virtual void GenerateSDK() = 0;

protected:
    virtual void InitPredefinedMembers() = 0;
    virtual void InitPredefinedFunctions() = 0;
};
