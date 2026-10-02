#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "Unreal/Enums.h"

/*
* Fills the usmap 'PackageVersioning' block, which describes how the target build saves packages:
*
* int32 bHasVersioning;                                // 1 if the engine version could be resolved, else 0
* if (bHasVersioning)
*     FEngineVersion EngineVersion;                    // ushort Major, ushort Minor, ushort Patch, uint32 Changelist, FString Branch
*     int32 FileVersionUE4;                            // 'GPackageFileUEVersion.FileVersionUE4'
*     int32 FileVersionUE5;                            // 'GPackageFileUEVersion.FileVersionUE5'
*     uint32 NumCustomVersions;                        // 'FCurrentCustomVersions::GetAll()' (FCustomVersionContainer, Optimized format)
*     for (int i = 0; i < NumCustomVersions; i++)
*         FGuid Key;                                   // uint32 A, B, C, D
*         int32 Version;
*     uint32 NetCL;                                    // 'FNetworkVersion::GetNetworkCompatibleChangelist()'
*
* The engine version is parsed from 'Settings::Generator::GameVersion'. The remaining three values are read from the
* live process: editor builds export them and can be looked up by name, while shipping builds only carry them as
* private constants that have to be located by scanning.
*/
class EngineVersioning
{
private:
    using StreamType = std::ofstream;

private:
    /* Mirrors the engine's 'FEngineVersion', parsed from 'Settings::Generator::GameVersion'. */
    struct FEngineVersion
    {
        uint16 Major = 0x0;
        uint16 Minor = 0x0;
        uint16 Patch = 0x0;
        /* 'FEngineVersion::Changelist', with the licensee bit ('0x80000000') already masked off. */
        uint32 Changelist = 0x0;
        std::string Branch;

        /* 'FEngineVersion::IsEmpty()' only checks the version components. */
        bool IsValid() const { return Major != 0x0; }
    };

    /* 'FCustomVersion' as it is laid out in memory (the 'FriendlyName' is not needed for the usmap). */
    struct FCustomVersionEntry
    {
        uint32 Key[0x4];
        int32 Version = 0x0;
    };

    /* Versioning read from the live process, used to fill the PackageVersioning block. */
    struct FRuntimeVersioning
    {
        int32 FileVersionUE4 = 0x0;
        int32 FileVersionUE5 = 0x0;
        std::vector<FCustomVersionEntry> CustomVersions;
        uint32 NetCL = 0x0;
    };

private:
    template<typename InStreamType, typename T>
    static void WriteToStream(InStreamType& InStream, T Value)
    {
        InStream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
    }

    /* UE FString save format (ANSI): positive length including the trailing NUL, then the bytes and a NUL. */
    template<typename InStreamType>
    static void WriteUEFString(InStreamType& InStream, const std::string& Value)
    {
        WriteToStream(InStream, static_cast<int32>(Value.size() + 1));

        if (!Value.empty())
            InStream.write(Value.c_str(), static_cast<std::streamsize>(Value.size()));

        WriteToStream(InStream, static_cast<char>(0x0));
    }

private:
    static FEngineVersion ParseEngineVersion();
    static FRuntimeVersioning ReadRuntimeVersioning(uint16 EngineMajorVersion);

    /* Reads an 'FCustomVersion' array that is already known to be at 'Elements'. */
    static bool TryReadCustomVersionArray(uintptr_t Elements, int32 Count, std::vector<FCustomVersionEntry>& OutCustomVersions);

    /* Locates the registered custom versions without an exported symbol, for shipping builds. */
    static bool FindCustomVersionsInDataSection(std::vector<FCustomVersionEntry>& OutCustomVersions);

    /* Locates 'GPackageFileUEVersion' through its callers when it is not exported. */
    static bool FindPackageFileVersionByScan(bool bIsUE5, int32& OutFileVersionUE4, int32& OutFileVersionUE5);

    /* Locates 'FNetworkVersion::GetNetworkCompatibleChangelist()' when it is not exported. */
    static void* FindNetworkCompatibleChangelistByScan();

public:
    /* 'bWriteEngineVersion' mirrors 'WrittenVersion >= EUsmapVersion::EngineVersioning'. */
    static void WritePackageVersioning(StreamType& InUsmap, bool bWriteEngineVersion);
};
