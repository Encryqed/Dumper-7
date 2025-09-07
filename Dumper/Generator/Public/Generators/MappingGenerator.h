#pragma once

#include <fstream>

#include "Unreal/ObjectArray.h"
#include "Wrappers/MemberWrappers.h"
#include "Wrappers/EnumWrapper.h"


/*
* USMAP-Header:
* 
* uint16 magic;
* uint8 version;
* if (version >= Packaging)
*     int32 bHasVersionInfo;
*     if (bHasVersionInfo)
*         int32 FileVersionUE4;
*         int32 FileVersionUE5;
*         uint32 NetCL;
* uint8 CompressionMethode;
* uint32 CompressedSize;
* uint32 DecompressedSize;
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
*     uint8 NumNamesInEnum;
*     for (int j = 0; j < NumNamesInEnum; j++)
*         int32 EnumMemberNameIdx;
* 
* uint32 StructCount;
* for (int i = 0; i < StructCount; i++)
*     int32 StructNameIdx;                                 // <-- START ParseStruct
*     int32 SuperTypeNameIdx;
*     uint16 PropertyCount;
*     uint16 SerializablePropertyCount;
*     for (int j = 0; j < SerializablePropertyCount; j++)
*         uint16 Index;                                    // <-- START ParsePropertyInfo
*         uint8 ArrayDim;
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
*             CALL ParsePropertyType;                       // <-- END ParsePropertyType, END ParsePropertyInfo, END ParseStruct
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

        Latest,
        LatestPlusOne,
    };

private:
    static constexpr uint16 UsmapFileMagic = 0x30C4;

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

private:
    /* Utility Functions */
    static EMappingsTypeFlags GetMappingType(UEProperty Property);
    static int32 AddNameToData(std::stringstream& NameTable, const std::string& Name);

private:
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
