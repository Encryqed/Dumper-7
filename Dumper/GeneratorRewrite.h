#pragma once
#include <filesystem>

#include "ObjectArray.h"
#include "DependencyManager.h"

namespace fs = std::filesystem;

struct StructInfo {};
struct ClassInfo {};
struct FunctionInfo {};

class GeneratorRewrite /* renamed to just 'Generator' once the legacy generator is removed */
{
protected:
    static inline DependencyManager Packages;

    static inline fs::path DumperFolder;

public:
    static void Init();

private:
    static bool SetupDumperFolder();

    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder);
    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder);

    static void GenerateStruct(const StructInfo& Struct) { };
    static void GenerateClass(const ClassInfo& Class) { };
    static void GenerateFunction(const FunctionInfo& Function) { };

public:
    template<typename GeneratorType>
    static void GenerateSDK() 
    { 
        if (DumperFolder.empty())
        {
            if (!SetupDumperFolder())
                return;
        }            

        if (!SetupFolders(GeneratorType::MainFolderName, GeneratorType::MainFolder, GeneratorType::SubfolderName, GeneratorType::Subfolder))
            return;

        GeneratorType::Generate(Packages);
    };

protected:
    static void InitPredefinedMembers() { };
    static void InitPredefinedFunctions() { };
};
