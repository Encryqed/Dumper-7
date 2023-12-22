#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "StructManager.h"
#include "MemberManager.h"
#include "HashStringTable.h"
#include "StructWrapper.h"
#include "MemberWrappers.h"

#include "GeneratorRewrite.h"

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
    struct FunctionInfo
    {
        bool bIsConst;
        bool bIsStatic;
        bool bIsUnrealFunc;
        bool bIsNativeUFunction;

        std::string RetType;
        std::string FuncNameWithParams;

        const std::string* BodyIfExisting = nullptr;
    };

private:
    using StreamType = std::ofstream;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

public: /* DEBUG */
    static std::string MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment);

    static std::string GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason);
    static std::string GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason);

    static std::string GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize);
    static std::string GenerateFunctionInHeader(const MemberManager& Members);
    static FunctionInfo GenerateFunctionInfo(const FunctionWrapper& Func);
    static void GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile);

    // return: In-header function declarations and inline functions
    static std::string GenerateFunctions(const MemberManager& Members, StreamType& FunctionFile, StreamType& ParamFile);

private: /* utility functions */
    static std::string GetMemberTypeString(const PropertyWrapper& MemberWrapper);
    static std::string GetMemberTypeString(UEProperty Member);
    static std::string GetStructPrefixedName(const StructWrapper& Struct);

public:
    static void Generate(const std::unordered_map<int32, PackageInfo>& Dependencies);

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};