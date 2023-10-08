#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "Nodes.h"
#include "HashStringTable.h"

namespace fs = std::filesystem;

struct PackageRewrite
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
    friend class Generator;

private:
    using StreamType = std::stringstream /*current config: debug, default: std::ofstream*/;

public:
    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    static std::string MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment);

    static std::string GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason);
    static std::string GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason);

    static std::string GenerateMembers(const std::vector<MemberNode>& Members, int32 SuperSize);
    static void GenerateStruct(StreamType& StructFile, const StructNode& Struct);
    static void GenerateClass(StreamType& ClassFile, const StructNode& Class);
    static void GenerateFunction(StreamType& FunctionFile, std::ofstream& ParamFile, const FunctionNode& Function);

public:
    static void Generate(const HashStringTable& NameTable, const DependencyManager& Dependencies);

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};