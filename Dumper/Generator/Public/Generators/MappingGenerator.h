#pragma once

#include <fstream>
#include <string>

#include "Unreal/ObjectArray.h"
#include "Wrappers/MemberWrappers.h"
#include "Wrappers/EnumWrapper.h"


/*
* USMAP-Header:
* 
* uint16 magic;
* uint8 version;                                           // Latest = ExtendedPropertyMetadata (6)
* if (version >= ExtendedPropertyMetadata)
*     FUsmapMetadata                                       // Tool, ToolVersion, CreatedAtUnix
* if (version >= PackageVersioning)
*     int32 bHasVersioning;                                // this dumper always writes 0
*     if (bHasVersioning)
*         if (version >= EngineVersioning)
*             FEngineVersion EngineVersion;
*         [FileVersionUE4/UE5 + CustomVersions + NetCL]     // not written
* uint8 CompressionMethod;
* uint32 CompressedSize;
* uint32 DecompressedSize;
* 
* FUsmapMetadata:
*     FString Tool;
*     FString ToolVersion;
*     int64 CreatedAtUnix;
* 
* 
* USMAP-Data:
* 
* uint32 NameCount;
* for (int i = 0; i < NameCount; i++)
*     [uint8|uint16] NameLength;
*     uint8 StringData[NameLength];
* 
* uint32 EnumCount;
* for (int i = 0; i < EnumCount; i++)
*     int32 EnumNameIdx;
*     if (version >= ExtendedPropertyMetadata)
*         int32 PackageNameIdx;                            // owner package, -1 if none
*     [uint8|uint16] NumNamesInEnum;              // u8 if version < LargeEnums, else u16
*     for (int j = 0; j < NumNamesInEnum; j++)
*         if (version >= ExplicitEnumValues)
*             uint64 EnumMemberValue;
*         int32 EnumMemberNameIdx;
* 
* if (version >= ExtendedPropertyMetadata)
*     uint32 FlagDictCount;
*     uint64 FlagDict[FlagDictCount];                      // unique EPropertyFlags, first-seen order
* 
* uint32 StructCount;
* for (int i = 0; i < StructCount; i++)
*     int32 StructNameIdx;                                 // <-- START ParseStruct
*     if (version >= ExtendedPropertyMetadata)
*         int32 PackageNameIdx;                            // owner package, -1 if none
*     int32 SuperTypeNameIdx;
*     uint16 PropertyCount;
*     uint16 SerializablePropertyCount;
*     for (int j = 0; j < SerializablePropertyCount; j++)
*         uint16 Index;                                    // <-- START ParsePropertyInfo
*         [uint8|uint16] ArrayDim;                         // u8 if version < ExtendedPropertyMetadata, else u16
*         int32 PropertyNameIdx;
*         uint8 MappingsTypeEnum;                         // <-- START ParsePropertyType      [[ByteProperty needs to be written as EnumProperty if it has an underlaying Enum]]
*         if (MappingsTypeEnum == EnumProperty || (MappingsTypeEnum == ByteProperty && UnderlayingEnum != null))
*             CALL ParsePropertyType;
*             int32 EnumName;
*         else if (MappingsTypeEnum == StructProperty)
*             int32 StructNameIdx;
*         else if (MappingsTypeEnum == (SetProperty | ArrayProperty | OptionalProperty))
*             CALL ParsePropertyType;
*         else if (MappingsTypeEnum == MapProperty)
*             CALL ParsePropertyType;
*             CALL ParsePropertyType;                       // <-- END ParsePropertyType
*         if (version >= ExtendedPropertyMetadata)
*             uint16 FlagIndex;
*/

class MappingGenerator
{
private:
    using StreamType = std::ofstream;

private:
    enum class EUsmapVersion : uint8
    {
        /* Initial format. */
        Initial,

        /* Adds package versioning to aid with compatibility */
        PackageVersioning,

        /* Adds support for 16-bit wide name-lengths (ushort/uint16) */
        LongFName,

        /* Adds support for enums with more than 255 values */
        LargeEnums,

        /* Adds support for explicit enum values */
        ExplicitEnumValues,

        /* Adds support for engine versioning information */
        EngineVersioning,

        /* Adds FUsmapMetadata, owner package names, EPropertyFlags dictionary, u16 ArrayDim, and FlagIndex */
        ExtendedPropertyMetadata,

        LatestPlusOne,
        Latest = LatestPlusOne - 1,
    };

private:
    static constexpr uint16 UsmapFileMagic = 0x30C4;
    static constexpr EUsmapVersion WrittenVersion = EUsmapVersion::Latest;

    static constexpr const char* MetadataToolName = "Dumper-7";
    static constexpr const char* MetadataToolVersion = "";

private:
    static inline uint64 NameCounter = 0x0;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "Mappings";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    template<typename InStreamType, typename T>
    static void WriteToStream(InStreamType& InStream, T Value)
    {
        InStream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
    }

    template<typename InStreamType>
    static void WriteToStream(InStreamType& InStream, const std::stringstream& Data)
    {
        InStream << Data.rdbuf();
    }

    template<typename InStreamType>
    static void WriteUEFString(InStreamType& InStream, const std::string& Value)
    {
        if (Value.empty())
        {
            WriteToStream(InStream, static_cast<int32>(0));
            return;
        }

        /* UE FString save: positive length includes the trailing NUL. */
        const int32 SaveNum = static_cast<int32>(Value.size() + 1);
        WriteToStream(InStream, SaveNum);
        InStream.write(Value.c_str(), SaveNum);
    }

private:
    /* Utility Functions */
    static EMappingsTypeFlags GetMappingType(UEProperty Property);
    static int32 AddNameToData(std::stringstream& NameTable, const std::string& Name);
    static void WriteOwnerPackageName(const UEObject& Object, std::stringstream& Data, std::stringstream& NameTable);

private:
    static bool ShouldExcludeEditorOnlyProperties();
    static bool ShouldWriteMappingProperty(const PropertyWrapper& Property);

    static void CollectPropertyFlags(const StructWrapper& Struct);
    static void CollectAllPropertyFlags();
    static void WriteFlagDictionary(std::stringstream& OutData);
    static void WriteUsmapMetadata(StreamType& InUsmap);

    static void GeneratePropertyType(UEProperty Property, std::stringstream& Data, std::stringstream& NameTable);
    static void GeneratePropertyInfo(const PropertyWrapper& Property, std::stringstream& Data, std::stringstream& NameTable, int32& Index);

    static void GenerateStruct(const StructWrapper& Struct, std::stringstream& Data, std::stringstream& NameTable);
    static void GenerateEnum(const EnumWrapper& Enum, std::stringstream& Data, std::stringstream& NameTable);

    static std::stringstream GenerateFileData();
    static void GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data);

public:
    static void Generate();

    /* Always empty, there are no predefined members for mappings */
    static void InitPredefinedMembers() { }
    static void InitPredefinedFunctions() { }
};
