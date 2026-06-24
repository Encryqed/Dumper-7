#pragma once

#include "Wrappers/StructWrapper.h"
#include "Wrappers/EnumWrapper.h"
#include "Wrappers/MemberWrappers.h"

#include <iostream>
#include <sstream>
#include <string>

#include "Unreal/ObjectArray.h"
#include "PredefinedMembers.h"

#include "IDAMappingLayouts.h"

class IDAMappingGenerator
{
private:
    using StreamType = std::ofstream;

private:
    static inline uint64 NameCounter = 0x0;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "IDAMappings";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    template<typename InStreamType, typename T>
    static void WriteToStream(InStreamType& InStream, T Value)
    {
        InStream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
    }

    template<typename InStreamType, typename T>
    static void WriteToStream(InStreamType& InStream, T* Value, int32 Size)
    {
        InStream.write(reinterpret_cast<const char*>(Value), Size);
    }

private:
    static IDAMappingsLayouts::StringOffset AddNameToData(std::stringstream& NameTable, const std::string& Name);

    static std::string MangleFunctionName(const std::string& ClassName, const std::string& FunctionName);
    static std::string MangleUFunctionName(const std::string& ClassName, const std::string& FunctionName);

    static std::string GetIDACppType(const PropertyWrapper& Member, bool& OutIsPtr);
    static std::string GetIDACppTypeForProperty(UEProperty Property, bool& OutIsPtr);
    static std::string BuildExecFuncSignature(UEFunction Func);

    static std::string GetStructPrefixedName(const StructWrapper& Struct);
    static std::string GetEnumPrefixedName(const EnumWrapper& Enum);

private:
    static void GenerateVTableName(std::stringstream& ExecFuncData, std::stringstream& ExecSigData, std::stringstream& NameData, UEObject DefaultObject);
    static void GenerateClassFunctions(std::stringstream& ExecFuncData, std::stringstream& ExecSigData, std::stringstream& NameData, UEClass Class);

    static void GenerateSingleMember(const PropertyWrapper& Member, std::stringstream& StructData, std::stringstream& NameData, int32 StructSize);
    static void GenerateSingleStruct(const StructWrapper& Struct, std::stringstream& StructData, std::stringstream& NameData);
    static void GenerateSingleEnum(const EnumWrapper& Enum, std::stringstream& EnumData, std::stringstream& NameData);

    static uint32 GeneratePredefinedTypes(std::stringstream& StructData, std::stringstream& NameData);
    static uint32 GenerateInternalEnums(std::stringstream& EnumData, std::stringstream& NameData);

public:
    static void Generate();

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions() {}
};
