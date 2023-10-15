#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "StructManager.h"
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

    static std::string GenerateMembers(UEStruct Struct, const std::vector<UEProperty>& Members, int32 SuperSize);
    static std::string GenerateFunctionInClass(const std::vector<UEFunction>& Functions);

    static void GenerateStruct(StreamType& StructFile, UEStruct Struct);
    static void GenerateClass(StreamType& ClassFile, UEClass Class);
    static void GenerateFunctionInCppFile(StreamType& FunctionFile, std::ofstream& ParamFile, const UEFunction& Function);

private: /* utility functions */
    static std::string GetMemberTypeString(UEProperty CurrentNode);
    static std::string GetStructPrefixedName(UEStruct Struct);

public:
    static void Generate(const DependencyManager& Dependencies);

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};