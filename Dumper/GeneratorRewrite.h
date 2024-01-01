#pragma once
#include <filesystem>

#include "ObjectArray.h"
#include "DependencyManager.h"
#include "MemberManager.h"

namespace fs = std::filesystem;

struct PackageInfo
{
    std::unordered_set<int32> PackageDependencies;

    DependencyManager Structs;
    DependencyManager Classes;
    std::vector<int32> Enums;
    std::vector<int32> Functions;
};

class GeneratorRewrite /* renamed to just 'Generator' once the legacy generator is removed */
{
private:
    friend class GeneratorRewriteTest;

protected:
    static inline std::unordered_map<int32, PackageInfo> Packages;

    static inline fs::path DumperFolder;

public:
    static void InitEngineCore();

private:
    static void InitInternal();

    static bool SetupDumperFolder();

    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder);
    static bool SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder);

    static std::unordered_map<int32, PackageInfo> GatherPackages();

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

        MemberManager::SetPredefinedMemberLookupPtr(&GeneratorType::PredefinedMembers);

        GeneratorType::Generate(Packages);
    };
};
