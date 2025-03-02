#pragma once

#include "Unreal/ObjectArray.h"

#include "Managers/StructManager.h"
#include "Managers/PackageManager.h"

#include "Wrappers/EnumWrapper.h"
#include "Wrappers/StructWrapper.h"
#include "Wrappers/MemberWrappers.h"

#include "Dumpspace/DSGen.h"


class DumpspaceGenerator
{
private:
    friend class CppGeneratorTest;
    friend class Generator;

private:
    using StreamType = std::ofstream;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "Dumpspace";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    static std::string GetStructPrefixedName(const StructWrapper& Struct);
    static std::string GetEnumPrefixedName(const EnumWrapper& Enum);

private:
    static std::string EnumSizeToType(const int32 Size);

private:
    static DSGen::EType GetMemberEType(const PropertyWrapper& Property);
    static DSGen::EType GetMemberEType(UEProperty Property);
    static std::string GetMemberTypeStr(UEProperty Property, std::string& OutExtendedType, std::vector<DSGen::MemberType>& OutSubtypes);
    static DSGen::MemberType GetMemberType(const StructWrapper& Struct);
    static DSGen::MemberType GetMemberType(UEProperty Property, bool bIsReference = false);
    static DSGen::MemberType GetMemberType(const PropertyWrapper& Property, bool bIsReference = false);
    static DSGen::MemberType ManualCreateMemberType(DSGen::EType Type, const std::string& TypeName, const std::string& ExtendedType = "");
    static void AddMemberToStruct(DSGen::ClassHolder& Struct, const PropertyWrapper& Property);

    static void RecursiveGetSuperClasses(const StructWrapper& Struct, std::vector<std::string>& OutSupers);
    static std::vector<std::string> GetSuperClasses(const StructWrapper& Struct);

private:
    static DSGen::ClassHolder GenerateStruct(const StructWrapper& Struct);
    static DSGen::EnumHolder GenerateEnum(const EnumWrapper& Enum);
    static DSGen::FunctionHolder GenearateFunction(const FunctionWrapper& Function);

    static void GeneratedStaticOffsets();

public:
    static void Generate();

    static void InitPredefinedMembers() { };
    static void InitPredefinedFunctions() { };
};