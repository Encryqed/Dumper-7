#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "Nodes.h"

namespace fs = std::filesystem;

struct Package
{
    // Function params that are & or * don't require sorting
    bool bHasClasses;
    bool bHasStructsOrEnums;
    bool bhasFunctions;
    DependencyManager Structs;
    DependencyManager Classes;
};

class CppGenerator
{
private:
    friend class CppGeneratorTest;

private:
    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    static std::string MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment);

    static std::string GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason);
    static std::string GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason);

    static std::string GenerateMember(const std::vector<MemberNode>& Members, int32 SuperSize);
    static void GenerateStruct(std::ofstream& StructFile, const StructNode& Struct);
    static void GenerateClass(std::ofstream& ClassFile, const StructNode& Class);
    static void GenerateFunction(std::ofstream& FunctionFile, std::ofstream& ParamFile, const FunctionNode& Function);

public:
    static void Generate(const DependencyManager& Dependencies);

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};