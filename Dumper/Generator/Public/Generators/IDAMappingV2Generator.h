#pragma once

#include "Wrappers/StructWrapper.h"
#include "Wrappers/EnumWrapper.h"
#include "Wrappers/MemberWrappers.h"

#pragma once

#include <iostream>
#include <string>

#include "Unreal/ObjectArray.h"
#include "PredefinedMembers.h"

#include "IDAMappingV2Layouts.hpp"

class IDAMappingV2Generator
{
private:
    enum class EIDAMappingsVersion : uint8_t
    {
        Initial = 1,
    };

private:
    static constexpr uint16 IdmapFileMagic = 'D7';

private:
    using StreamType = std::ofstream;

private:
    static inline uint64 NameCounter = 0x0;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "IDAMappingsV2";
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

private:
    static void WriteReadMe(StreamType& ReadMe);

    static void GenerateVTableName(std::stringstream& NameData, UEObject DefaultObject);
    static void GenerateClassFunctions(std::stringstream& NameData, UEClass Class);

    static void GenerateSingleMember(const PropertyWrapper& Member, std::stringstream& StructData, std::stringstream& NameData);
    static void GenerateSingleStruct(const StructWrapper& Struct, std::stringstream& StructData, std::stringstream& NameData);
    static void GenerateSingleEnum(StreamType& IdmapFile, const EnumWrapper& Enum, std::stringstream& SerializedEnumData);

	static std::vector<IDAMappingsLayouts::NamedVariable> GenerateNamedVariables(StreamType& IdmapFile, std::stringstream& NameData);

    void GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data);

public:
    static void Generate();

    /* Always empty, there are no predefined members for IDAMappings */
    static void InitPredefinedMembers() {}
    static void InitPredefinedFunctions() {}
};
