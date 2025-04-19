#pragma once

#include <filesystem>
#include <fstream>

#include "Managers/DependencyManager.h"
#include "Managers/StructManager.h"
#include "Managers/MemberManager.h"
#include "Wrappers/StructWrapper.h"
#include "Wrappers/MemberWrappers.h"
#include "Wrappers/EnumWrapper.h"
#include "Managers/PackageManager.h"

#include "HashStringTable.h"
#include "Generator.h"


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

        UnrealContainers,
        UnicodeLib,

        PropertyFixup,
        SdkHpp,

        DebugAssertions,
    };

private:
    using StreamType = std::ofstream;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    static inline std::vector<PredefinedStruct> PredefinedStructs;

private:
    static std::string MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment);
    static std::string MakeMemberStringWithoutName(const std::string& Type);

    static std::string GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason);
    static std::string GenerateBitPadding(uint8 UnderlayingSizeBytes, const uint8 PrevBitPropertyEndBit, const int32 Offset, const int32 PadSize, std::string&& Reason);

    static std::string GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize, int32 SuperLastMemberEnd, int32 SuperAlign, int32 PackageIndex = -1);
    static FunctionInfo GenerateFunctionInfo(const FunctionWrapper& Func);

    // return: In-header function declarations and inline functions
    static std::string GenerateSingleFunction(const FunctionWrapper& Func, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile);
    static std::string GenerateFunctions(const StructWrapper& Struct, const MemberManager& Members, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile);

    static void GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile, int32 PackageIndex = -1, const std::string& StructNameOverride = std::string());

    static void GenerateEnum(const EnumWrapper& Enum, StreamType& StructFile);

private: /* utility functions */
    static std::string GetMemberTypeString(const PropertyWrapper& MemberWrapper, int32 PackageIndex = -1, bool bAllowForConstPtrMembers = false /* const USomeClass* Member; */);
    static std::string GetMemberTypeString(UEProperty Member, int32 PackageIndex = -1, bool bAllowForConstPtrMembers = false);
    static std::string GetMemberTypeStringWithoutConst(UEProperty Member, int32 PackageIndex = -1);

    static std::string GetFunctionSignature(UEFunction Func);

    static std::string GetStructPrefixedName(const StructWrapper& Struct);
    static std::string GetEnumPrefixedName(const EnumWrapper& Enum);
    static std::string GetEnumUnderlayingType(const EnumWrapper& Enm);

    static std::string GetCycleFixupType(const StructWrapper& Struct, bool bIsForInheritance);

    static std::unordered_map<std::string, UEProperty> GetUnknownProperties();

private:
    static void GenerateEnumFwdDeclarations(StreamType& ClassOrStructFile, PackageInfoHandle Package, bool bIsClassFile);

private:
    static void GenerateNameCollisionsInl(StreamType& NameCollisionsFile);
    static void GeneratePropertyFixupFile(StreamType& PropertyFixup);
    static void GenerateDebugAssertions(StreamType& AssertionStream);
    static void WriteFileHead(StreamType& File, PackageInfoHandle Package, EFileType Type, const std::string& CustomFileComment = "", const std::string& CustomIncludes = "");
    static void WriteFileEnd(StreamType& File, EFileType Type);

    static void GenerateSDKHeader(StreamType& SdkHpp);

    static void GenerateBasicFiles(StreamType& BasicH, StreamType& BasicCpp);

    /*
    * Creates the UnrealContainers.hpp file (without allocation code) for the SDK. 
    * File contains the implementation of TArray, FString, TSparseArray, TSet, TMap and iterators for them
    *
    * See https://github.com/Fischsalat/UnrealContainers/blob/master/UnrealContainers/UnrealContainersNoAlloc.h 
    */
    static void GenerateUnrealContainers(StreamType& UEContainersHeader);

    /*
    * Creates the UtfN.hpp file for the SDK.
    *
    * See https://github.com/Fischsalat/UTF-N
    */
    static void GenerateUnicodeLib(StreamType& UnicodeLib);

public:
    static void Generate();

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};