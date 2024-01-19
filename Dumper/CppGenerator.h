#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "StructManager.h"
#include "MemberManager.h"
#include "HashStringTable.h"
#include "StructWrapper.h"
#include "MemberWrappers.h"
#include "EnumWrapper.h"
#include "PackageManager.h"

#include "GeneratorRewrite.h"

namespace fs = std::filesystem;


class CppGenerator
{
private:
    friend class CppGeneratorTest;
    friend class Generator;

private:
    struct ParamInfo
    {
        bool bIsOutPtr;
        bool bIsOutRef;
        bool bIsMoveParam;
        bool bIsRetParam;
        bool bIsConst;
        EPropertyFlags PropFlags;

        std::string Type;
        std::string Name;
    };

    struct FunctionInfo
    {
        bool bIsReturningVoid;
        EFunctionFlags FuncFlags = EFunctionFlags::None;

        std::string RetType;
        std::string FuncNameWithParams;

        std::vector<ParamInfo> UnrealFuncParams; // for unreal-functions only
    };

    enum class EFileType
    {
        Classes,
        Structs,
        Parameters,
        Functions,

        NameCollisionsInl,

        BasicHpp,
        BasicCpp,

        PropertyFixup,
    };

private:
    using StreamType = std::ofstream;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

    static inline std::vector<PredefinedStruct> PredefinedStructs;

public: /* DEBUG */
    static std::string MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment);
    static std::string MakeMemberStringWithoutName(const std::string& Type, std::string&& Comment);

    static std::string GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason);
    static std::string GenerateBitPadding(uint8 UnderlayingSizeBytes, const int32 Offset, const int32 PadSize, std::string&& Reason);

    static std::string GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize);
    static std::string GenerateFunctionInHeader(const MemberManager& Members);
    static FunctionInfo GenerateFunctionInfo(const FunctionWrapper& Func);

    // return: In-header function declarations and inline functions
    static std::string GenerateSingleFunction(const FunctionWrapper& Func, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile);
    static std::string GenerateFunctions(const StructWrapper& Struct, const MemberManager& Members, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile);

    static void GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile);

    static void GenerateEnum(const EnumWrapper& Enum, StreamType& StructFile);

private: /* utility functions */
    static std::string GetMemberTypeString(const PropertyWrapper& MemberWrapper, bool bAllowForConstPtrMembers = false /* const USomeClass* Member; */);
    static std::string GetMemberTypeString(UEProperty Member, bool bAllowForConstPtrMembers = false);
    static std::string GetMemberTypeStringWithoutConst(UEProperty Member);

    static std::string GetStructPrefixedName(const StructWrapper& Struct);
    static std::string GetEnumPrefixedName(const EnumWrapper& Enum);

    static std::unordered_map<std::string /* Name */, int32 /* Size */> GetUnknownProperties();

private:
    static void GenerateNameCollisionsInl(StreamType& NameCollisionsFile);
    static void GeneratePropertyFixupFile(StreamType& PropertyFixup);
    static void WriteFileHead(StreamType& File, PackageInfoHandle Package, EFileType Type, const std::string& CustomFileComment = "", const std::string& CustomIncludes = "");
    static void WriteFileEnd(StreamType& File, EFileType Type);

    static void GenerateBasicFiles(StreamType& BasicH, StreamType& BasicCpp);

public:
    static void Generate();

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};