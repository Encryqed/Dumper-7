#pragma once

#include <iostream>
#include <string>

#include "Unreal/ObjectArray.h"
#include "PredefinedMembers.h"


class IDAMappingGenerator
{
public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "IDAMappings";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    using StreamType = std::ofstream;

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
    static std::string MangleFunctionName(const std::string& ClassName, const std::string& FunctionName);

private:
    static void WriteReadMe(StreamType& ReadMe);

    static void GenerateVTableName(StreamType& IdmapFile, UEObject DefaultObject);
    static void GenerateClassFunctions(StreamType& IdmapFile, UEClass Class);

public:
    static void Generate();

    /* Always empty, there are no predefined members for IDAMappings */
    static void InitPredefinedMembers() { }
    static void InitPredefinedFunctions() { }
};