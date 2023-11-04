#pragma once
#include <filesystem>

#include "ObjectArray.h"
#include "DependencyManager.h"
#include "HashStringTable.h"

namespace fs = std::filesystem;

struct PackageMembers
{
    DependencyManager Structs;
    DependencyManager Classes;
    std::vector<int32> Enums;
    std::vector<int32> Functions;
};

class GeneratorRewrite /* renamed to just 'Generator' once the legacy generator is removed */
{
protected:
    static inline DependencyManager Packages;

    static inline fs::path DumperFolder;

public:
    static void InitCore();
    static void Init();

private:
    static bool SetupDumperFolder();

    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder);
    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder);

    static std::unordered_map<int32, PackageMembers> GatherPackages();

public:
    template<typename GeneratorType>
    static void Generate() 
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
};
