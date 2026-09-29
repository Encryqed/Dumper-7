#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Generators/EngineVersioning.h"
#include "Platform.h"

#include "../Settings.h"

namespace
{
    /* Cooked Editor Symbols */
    constexpr auto GPackageFileUEVersionExport = "?GPackageFileUEVersion@@3UFPackageFileVersion@@B";
    constexpr auto FCurrentCustomVersionsGetAllExport = "?GetAll@FCurrentCustomVersions@@SA?AVFCustomVersionContainer@@XZ";
    constexpr auto FNetworkVersionGetNetworkCompatibleChangelistExport = "?GetNetworkCompatibleChangelist@FNetworkVersion@@SAIXZ";

    /* 'FEngineVersion::Changelist' masks off the licensee bit */
    constexpr uint32 ChangelistLicenseeMask = 0x7FFFFFFF;

    /* 'GPackageFileUEVersion' is a 'const FPackageFileVersion', which is two consecutive 'int32's. */
    constexpr uint32 FPackageFileVersionSize = 0x8;

    /* 'FCustomVersionContainer' only wraps a 'TArray<FCustomVersion>'. */
    constexpr uint32 FCustomVersionContainerSize = 0x10;
    constexpr uint32 FCustomVersionContainerNumOffset = 0x8;
    constexpr uint32 FCustomVersionContainerMaxOffset = 0xC;

    /*
    * 'FCustomVersion' always starts with the 'FGuid Key' and keeps the 'int32 Version' right after it, so those two
    * fields are enough to fill the usmap and their offsets never change.
    */
    constexpr uint32 FCustomVersionKeySize = 0x10;
    constexpr uint32 FCustomVersionReferenceCountOffset = 0x14;

    /*
    * The fields after the version are what makes 'sizeof(FCustomVersion)' vary, so the stride has to be detected:
    *
    * 0x20 - 'FGuid Key'(0x10) + 'int32 Version' + 'int32 ReferenceCount' + 'FName FriendlyName'(0x8, no 'Validator' field)
    * 0x24 - same as 0x20, but 'FName' is 0xC bytes because 'WITH_CASE_PRESERVING_NAME' adds a 'DisplayIndex'
    * 0x28 - same as 0x20, but with the 0x8-byte 'CustomVersionValidatorFunc Validator' before the 'FName'
    * 0x30 - same as 0x28, but 'FName' is 0x10 bytes because 'WITH_CASE_PRESERVING_NAME' adds a 'DisplayIndex'
    */
    constexpr uint32 FCustomVersionStrides[] = {0x20, 0x24, 0x28, 0x30};

    /* A container holding more than this, or spanning more than this many bytes, is garbage. */
    constexpr int32 MaxSaneCustomVersionCount = 0x1000;
    constexpr uint64 MaxSaneCustomVersionBytes = 0x400000;

    /*
    * Candidates come from arbitrary bytes in '.data', so the pointer they carry is only worth dereferencing once it
    * is known to be a plausible user-mode address. This rejects almost every candidate before the costlier checks,
    * which matters because 'IsAddressInAnyModule' walks every loaded module.
    */
    constexpr uintptr_t MinSaneAddress = 0x10000;
    constexpr uintptr_t MaxSaneAddress = 0x7FFFFFFFFFFF;

    /* Custom versions are small, incremental constants. This is only here to reject obvious garbage. */
    constexpr int32 MaxSaneCustomVersion = 0x10000;

    /* How far 'Max' may sit above 'Num' before the container stops looking like a real 'TArray'. */
    constexpr int32 MaxSaneCustomVersionSlack = 0x400;

    /* Fewer than this is not a plausible registry, and scanning for it is not worth the false positives. */
    constexpr int32 MinSaneCustomVersionCount = 0x5;

    /*
    * 'GPackageFileUEVersion' cannot be referenced by name in a shipping build, but it can be found through
    * 'PACKAGE_FILE_TAG': the package reader compares every summary against that constant, so the functions
    * doing so also load the version. Materialised as a little-endian imm32 it is the seed for the search.
    */
    constexpr uint8 PackageFileTagBytes[0x4] = {0xC1, 0x83, 0x2A, 0x9E};
    constexpr int32 MinPackageFileVersion = 342;
    constexpr int32 MaxPackageFileVersion = 600;
    constexpr uintptr_t PackageFileTagScanWindow = 0x800;

    /* 'call rel32' plus the distance searched backwards from the log reference to reach it. */
    constexpr uint8 RelativeCallOpcode = 0xE8;
    constexpr uintptr_t RelativeCallInstructionSize = 0x5;
    constexpr uintptr_t NetworkChangelistBackwardScanRange = 0x40;

    /*
    * Reads a 'TArray' by dereferencing a pointer that was found by scanning, so the entire span has to be proven
    * readable before it is touched. Every page is checked rather than just the two ends: a candidate can start
    * inside a valid allocation and run off the end of it, and an incorrect stride only makes that more likely.
    */
    bool IsSpanReadable(const uintptr_t Start, const uint64 Size)
    {
        constexpr uintptr_t PageSize = 0x1000;

        if (Start == 0x0 || Size == 0x0)
            return false;

        const uintptr_t End = Start + Size;

        if (End < Start)
            return false;

        for (uintptr_t Page = Start & ~(PageSize - 0x1); Page < End; Page += PageSize)
        {
            if (Platform::IsBadReadPtr(Page))
                return false;
        }

        return true;
    }

    /*
    * A section can contain pages that a protector left unreadable, so every byte-level scan has to walk the readable
    * spans only; touching the section as one flat block would fault on the first unreadable page.
    */
    template<typename CallbackType>
    void ForEachReadableRange(const uintptr_t Start, const uint64 Size, CallbackType&& Callback)
    {
        constexpr uintptr_t PageSize = 0x1000;

        if (Start == 0x0 || Size == 0x0)
            return;

        const uintptr_t End = Start + Size;

        if (End < Start)
            return;

        uintptr_t RangeStart = Start;

        for (uintptr_t Page = Start; Page < End; Page += PageSize)
        {
            if (!Platform::IsBadReadPtr(Page))
                continue;

            if (Page > RangeStart)
                Callback(RangeStart, Page - RangeStart);

            RangeStart = Page + PageSize;
        }

        if (End > RangeStart)
            Callback(RangeStart, End - RangeStart);
    }

    /* A stripped view of the main module's '.text', which the byte-level scans walk directly. */
    struct FTextSection
    {
        const uint8* Begin = nullptr;
        uint64 Size = 0x0;

        [[nodiscard]] bool IsValid() const { return Begin != nullptr && Size > 0x0; }
    };

    FTextSection GetTextSection()
    {
        const uintptr_t ModuleBase = Platform::GetModuleBase();

        if (ModuleBase == 0x0)
            return {};

        const auto* DosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(ModuleBase);

        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return {};

        const auto* NtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>(ModuleBase + DosHeader->e_lfanew);

        if (NtHeaders->Signature != IMAGE_NT_SIGNATURE || NtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            return {};

        const IMAGE_SECTION_HEADER* Section = IMAGE_FIRST_SECTION(NtHeaders);

        for (WORD i = 0x0; i < NtHeaders->FileHeader.NumberOfSections; ++i, ++Section)
        {
            if (std::strncmp(reinterpret_cast<const char*>(Section->Name), ".text", 7) != 0)
                continue;

            return {.Begin = reinterpret_cast<const uint8*>(ModuleBase + Section->VirtualAddress), .Size = Section->Misc.VirtualSize};
        }

        return {};
    }

    /*
    * 'FPackageFileVersion' is two consecutive 'int32's, and a UE4 build always keeps the UE5 field at zero.
    * The UE4 object version is a small, slowly increasing constant, which is what rejects garbage reads.
    */
    /*
    * 'FPackageFileVersion' only exists from UE5 ('UE4.27'); older builds keep a single 'int32 GPackageFileUEVersion'
    * instead, where the following word belongs to whatever global sits next to it. The engine major version therefore
    * decides which fields are meaningful, otherwise the unrelated neighbour rejects every correct (UE4) candidate.
    */
    bool TryReadPackageFileVersion(const uintptr_t Address, const bool bIsUE5, int32& OutFileVersionUE4, int32& OutFileVersionUE5)
    {
        const uint32 Span = bIsUE5 ? FPackageFileVersionSize : sizeof(int32);

        if (Address == 0x0 || !IsSpanReadable(Address, Span))
            return false;

        const int32 FileVersionUE4 = *reinterpret_cast<const int32*>(Address);

        if (FileVersionUE4 < MinPackageFileVersion || FileVersionUE4 > MaxPackageFileVersion)
            return false;

        if (bIsUE5)
        {
            /*
            * A UE5 build tracks the UE5 version in the following field; the UE4 field only stays set while the
            * build is still UE4-compatible, so a non-zero UE5 field means this is not the engine's own constant.
            */
            const int32 FileVersionUE5 = *reinterpret_cast<const int32*>(Address + 0x4);

            if (FileVersionUE5 != 0x0)
                return false;

            OutFileVersionUE5 = FileVersionUE5;
        }
        else
        {
            /* There is no UE5 field to read, so the usmap gets a zero there. */
            OutFileVersionUE5 = 0x0;
        }

        OutFileVersionUE4 = FileVersionUE4;

        return true;
    }

    /*
    * A real 'FGuid' is a random 128-bit value, so neither of its two 64-bit halves may fall inside a module.
    * A misaligned read through a struct of pointers instead yields the pointer itself, which does land in a module.
    */
    bool IsGarbageGuid(const uint32* Key)
    {
        const uint64 LowHalf = (static_cast<uint64>(Key[0x1]) << 32) | Key[0x0];
        const uint64 HighHalf = (static_cast<uint64>(Key[0x3]) << 32) | Key[0x2];

        return Platform::IsAddressInAnyModule(LowHalf) || Platform::IsAddressInAnyModule(HighHalf);
    }

    /* 'FGuid::ToString()' */
    std::string FormatCustomVersionKey(const uint32 (&Key)[0x4])
    {
        return std::format("{:08X}-{:08X}-{:08X}-{:08X}", Key[0x0], Key[0x1], Key[0x2], Key[0x3]);
    }
}


EngineVersioning::FEngineVersion EngineVersioning::ParseEngineVersion()
{
    FEngineVersion Version;

    /* 'Settings::Generator::GameVersion' holds 'UKismetSystemLibrary::GetEngineVersion()' (an FEngineVersion::ToString()). */
    const std::string& VersionString = Settings::Generator::GameVersion;

    size_t Pos = 0;
    auto ReadNumber = [&VersionString, &Pos](uint64& OutValue) -> bool
    {
        const size_t Start = Pos;
        uint64 Value = 0x0;

        while (Pos < VersionString.size() && VersionString[Pos] >= '0' && VersionString[Pos] <= '9')
        {
            Value = Value * 10 + static_cast<uint64>(VersionString[Pos] - '0');
            Pos++;
        }

        OutValue = Value;
        return Pos != Start;
    };

    auto Consume = [&VersionString, &Pos](const char Expected)
    {
        if (Pos < VersionString.size() && VersionString[Pos] == Expected)
        {
            Pos++;
            return true;
        }

        return false;
    };

    /* 'FEngineVersion::ToString()' -> "Major.Minor.Patch-Changelist+Branch", where the trailing components are optional. */
    uint64 Major = 0x0;
    uint64 Minor = 0x0;
    uint64 Patch = 0x0;
    uint64 Changelist = 0x0;

    if (!ReadNumber(Major))
        return Version;

    if (Consume('.') && !ReadNumber(Minor))
        return Version;

    if (Consume('.') && !ReadNumber(Patch))
        return Version;

    if (Consume('-'))
        ReadNumber(Changelist);

    if (Consume('+'))
        Version.Branch = VersionString.substr(Pos);

    /* 'FEngineVersion' stores the version components as ushort and the changelist as uint32. */
    auto ClampToUInt16 = [](const uint64 Value) -> uint16
    {
        return static_cast<uint16>(Value > 0xFFFF ? 0xFFFF : Value);
    };
    auto ClampToUInt32 = [](const uint64 Value) -> uint32
    {
        return static_cast<uint32>(Value > 0xFFFFFFFF ? 0xFFFFFFFF : Value);
    };

    Version.Major = ClampToUInt16(Major);
    Version.Minor = ClampToUInt16(Minor);
    Version.Patch = ClampToUInt16(Patch);
    Version.Changelist = ClampToUInt32(Changelist) & ChangelistLicenseeMask;

    return Version;
}

EngineVersioning::FRuntimeVersioning EngineVersioning::ReadRuntimeVersioning(const uint16 EngineMajorVersion)
{
    FRuntimeVersioning Versioning;

    /* 'FPackageFileVersion' replaced the single 'int32 GPackageFileUEVersion' in UE5, which changes which fields can be read. */
    const bool bIsUE5 = EngineMajorVersion >= 0x5;

    constexpr bool bSupportsInProcessCalls =
#ifdef _WIN64
        true;
#else
    /* Returning a non-trivial type by value uses an ABI that is not implemented here. */
    false;
#endif

    /*
    * 'GPackageFileUEVersion' is the engine's 'FPackageFileVersion' and gets baked into every saved package.
    * It is only readable by name in builds that export it, so shipping builds locate it by scanning instead.
    */
    if (const void* PackageFileVersion = Platform::GetAddressOfExportedFunction(Settings::General::DefaultModuleName, GPackageFileUEVersionExport))
    {
        static_assert(FPackageFileVersionSize == 0x8 && sizeof(int32) == 0x4);

        Versioning.FileVersionUE4 = *static_cast<const int32*>(PackageFileVersion);
        Versioning.FileVersionUE5 = *reinterpret_cast<const int32*>(static_cast<const uint8*>(PackageFileVersion) + sizeof(int32));

        std::cerr << std::format("MappingGeneration: Found '{}' at 0x{:X}\n", GPackageFileUEVersionExport, reinterpret_cast<uintptr_t>(PackageFileVersion));
        std::cerr << std::format("MappingGeneration: FileVersionUE4 = {}, FileVersionUE5 = {} (exported)\n", Versioning.FileVersionUE4, Versioning.FileVersionUE5);
    }
    else if (FindPackageFileVersionByScan(bIsUE5, Versioning.FileVersionUE4, Versioning.FileVersionUE5))
    {
        std::cerr << std::format("MappingGeneration: FileVersionUE4 = {}, FileVersionUE5 = {} (scanned)\n", Versioning.FileVersionUE4, Versioning.FileVersionUE5);
    }
    else
    {
        std::cerr << std::format("MappingGeneration: '{}' was neither exported nor found by scanning, FileVersionUE4/UE5 stay 0\n", GPackageFileUEVersionExport);
    }

    if constexpr (bSupportsInProcessCalls)
    {
        /*
        * 'FCurrentCustomVersions::GetAll()' returns an 'FCustomVersionContainer' by value and MSVC returns non-trivial
        * classes through a hidden pointer, so that pointer has to be supplied as the first (and only) argument.
        */

        if (const void* GetAllCustomVersions = Platform::GetAddressOfExportedFunction(Settings::General::DefaultModuleName, FCurrentCustomVersionsGetAllExport))
        {
            std::cerr << std::format("MappingGeneration: Found '{}' at 0x{:X}\n", FCurrentCustomVersionsGetAllExport, reinterpret_cast<uintptr_t>(GetAllCustomVersions));

            alignas(0x8) uint8 Container[FCustomVersionContainerSize] = {};
            reinterpret_cast<void (*)(void*)>(const_cast<void*>(GetAllCustomVersions))(Container);

            const uintptr_t Elements = *reinterpret_cast<const uintptr_t*>(Container);
            const int32 Count = *reinterpret_cast<const int32*>(Container + FCustomVersionContainerNumOffset);
            const int32 Max = *reinterpret_cast<const int32*>(Container + FCustomVersionContainerMaxOffset);

            std::cerr << std::format("MappingGeneration: FCustomVersionContainer.ElementData = 0x{:X}, Num = {}, Max = {}\n", Elements, Count, Max);

            if (!TryReadCustomVersionArray(Elements, Count, Versioning.CustomVersions))
                std::cerr << "MappingGeneration: The exported FCustomVersionContainer could not be read\n";
        }
        else
        {
            /* No export means a shipping build, where the registry has to be found by scanning instead. */
            std::cerr << std::format("MappingGeneration: '{}' was not exported, scanning for the custom version registry\n", FCurrentCustomVersionsGetAllExport);

            if (!FindCustomVersionsInDataSection(Versioning.CustomVersions))
                std::cerr << "MappingGeneration: The custom version registry could not be found\n";
        }

        /*
        * 'FNetworkVersion::GetNetworkCompatibleChangelist()' is the changelist the network protocol version is
        * based on. It is a function, so the exported name is used when present and located by scanning otherwise.
        */
        const void* GetNetworkCompatibleChangelist = Platform::GetAddressOfExportedFunction(Settings::General::DefaultModuleName, FNetworkVersionGetNetworkCompatibleChangelistExport);

        if (GetNetworkCompatibleChangelist)
        {
            std::cerr << std::format("MappingGeneration: Found '{}' at 0x{:X}\n", FNetworkVersionGetNetworkCompatibleChangelistExport, reinterpret_cast<uintptr_t>(GetNetworkCompatibleChangelist));
        }
        else
        {
            std::cerr << std::format("MappingGeneration: '{}' was not exported, scanning for it\n", FNetworkVersionGetNetworkCompatibleChangelistExport);

            GetNetworkCompatibleChangelist = FindNetworkCompatibleChangelistByScan();

            if (GetNetworkCompatibleChangelist)
                std::cerr << std::format("MappingGeneration: Scanned '{}' at 0x{:X}\n", FNetworkVersionGetNetworkCompatibleChangelistExport, reinterpret_cast<uintptr_t>(GetNetworkCompatibleChangelist));
        }

        if (GetNetworkCompatibleChangelist)
            Versioning.NetCL = reinterpret_cast<uint32 (*)()>(const_cast<void*>(GetNetworkCompatibleChangelist))();
        else
            std::cerr << std::format("MappingGeneration: '{}' was neither exported nor found by scanning, assuming NetCL = 0\n", FNetworkVersionGetNetworkCompatibleChangelistExport);
    }

    std::cerr << std::format("MappingGeneration: FileVersionUE4 = {}, FileVersionUE5 = {}\n", Versioning.FileVersionUE4, Versioning.FileVersionUE5);
    std::cerr << std::format("MappingGeneration: NumCustomVersions = {}, NetCL = {}\n", Versioning.CustomVersions.size(), Versioning.NetCL);

    if (Settings::Debug::bShouldPrintMappingDebugData)
    {
        for (const auto& [Key, Version] : Versioning.CustomVersions)
        {
            std::cerr << std::format("MappingGeneration:     CustomVersion {} = {}\n", FormatCustomVersionKey(Key), Version);
        }
    }

    return Versioning;
}

bool EngineVersioning::TryReadCustomVersionArray(const uintptr_t Elements, const int32 Count, std::vector<FCustomVersionEntry>& OutCustomVersions)
{
    /* The container is only a candidate at all if the pointer it holds is worth dereferencing. */
    if (Count < MinSaneCustomVersionCount
        || Count > MaxSaneCustomVersionCount
        || Elements < MinSaneAddress
        || Elements > MaxSaneAddress
        || Platform::IsAddressInAnyModule(Elements))
        return false;

    const auto ElementsPtr = reinterpret_cast<const uint8*>(Elements);

    /*
    * The stride is unknown, so every candidate is tried and the first one whose entire array validates wins.
    * Validating the whole array is what makes this reliable: a wrong stride starts drifting on element two.
    */
    for (const uint32 Stride : FCustomVersionStrides)
    {
        const uint64 Span = static_cast<uint64>(Count) * Stride;

        if (Span > MaxSaneCustomVersionBytes || !IsSpanReadable(Elements, Span))
            continue;

        std::vector<FCustomVersionEntry> Parsed;
        Parsed.reserve(static_cast<size_t>(Count));

        bool bValid = true;

        for (int32 i = 0x0; i < Count; ++i)
        {
            constexpr uint32 FCustomVersionVersionOffset = 0x10;
            const uint8* const Element = ElementsPtr + static_cast<uint64>(i) * Stride;

            const int32 Version = *reinterpret_cast<const int32*>(Element + FCustomVersionVersionOffset);
            const int32 ReferenceCount = *reinterpret_cast<const int32*>(Element + FCustomVersionReferenceCountOffset);

            /* Statically registered versions are registered exactly once, and their version stays small. */
            if (ReferenceCount != 0x1 || Version < 0x0 || Version > MaxSaneCustomVersion)
            {
                bValid = false;
                break;
            }

            const auto Key = reinterpret_cast<const uint32*>(Element);

            if (IsGarbageGuid(Key))
            {
                bValid = false;
                break;
            }

            FCustomVersionEntry CustomVersion;
            memcpy(CustomVersion.Key, Key, FCustomVersionKeySize);
            CustomVersion.Version = Version;

            Parsed.push_back(CustomVersion);
        }

        if (bValid)
        {
            OutCustomVersions = std::move(Parsed);

            std::cerr << std::format("MappingGeneration: Read {} custom version(s) at 0x{:X} using sizeof(FCustomVersion) = 0x{:X}\n",
                                     Count, Elements, Stride);

            return true;
        }
    }

    return false;
}

bool EngineVersioning::FindCustomVersionsInDataSection(std::vector<FCustomVersionEntry>& OutCustomVersions)
{
    /*
    * 'FStaticCustomVersionRegistry::Registered' is a function-local static, so its 'FCustomVersionContainer'
    * lives in this module's writable data and cannot be reached by name. It is a 'TArray<FCustomVersion>',
    * which means {ElementData, Num, Max}, so every aligned triple in '.data' is a candidate.
    */
    const SectionInfo DataSection = Platform::GetSectionInfo(".data");

    if (!DataSection.IsValid())
    {
        std::cerr << "MappingGeneration: The '.data' section could not be located\n";
        return false;
    }

    bool bFound = false;

    /* Returning false keeps the iteration going, the scan stops by setting 'bFound'. */
    Platform::IterateSectionWithCallback(DataSection, [&OutCustomVersions, &bFound](void* Address) -> bool
    {
        const auto Candidate = static_cast<const uint8*>(Address);

        const uintptr_t Elements = *reinterpret_cast<const uintptr_t*>(Candidate);
        const int32 Count = *reinterpret_cast<const int32*>(Candidate + FCustomVersionContainerNumOffset);
        const int32 Max = *reinterpret_cast<const int32*>(Candidate + FCustomVersionContainerMaxOffset);

        /* Cheap rejections first, the vast majority of triples fail right here. */
        if (Count < MinSaneCustomVersionCount || Count > MaxSaneCustomVersionCount)
            return false;

        if (Max < Count || Max > Count + MaxSaneCustomVersionSlack)
            return false;

        if (!TryReadCustomVersionArray(Elements, Count, OutCustomVersions))
            return false;

        std::cerr << std::format("MappingGeneration: Found the custom version registry at 0x{:X} (Num = {}, Max = {})\n",
                                 reinterpret_cast<uintptr_t>(Address), Count, Max);

        bFound = true;
        return true;
    }, 0x8);

    return bFound;
}

bool EngineVersioning::FindPackageFileVersionByScan(const bool bIsUE5, int32& OutFileVersionUE4, int32& OutFileVersionUE5)
{
    /*
    * 'GPackageFileUEVersion' is a private constant, so it has no name to look up. What it does have is a caller:
    * the package reader compares every summary against 'PACKAGE_FILE_TAG' and then consults the version, so the
    * functions that materialise the tag also load the version. Seeding on the tag therefore narrows the search
    * from the whole image down to the handful of functions that serialise packages.
    *
    * Within those functions every 4-byte window is tried as if it were a RIP-relative displacement; only the
    * windows that really are one resolve to an 'FPackageFileVersion', and the real global collects a vote from
    * each function that reads it while coincidental hits do not repeat. The most voted target wins.
    */
    const FTextSection Text = GetTextSection();

    if (!Text.IsValid())
    {
        std::cerr << "MappingGeneration: The '.text' section could not be located for the file version scan\n";
        return false;
    }

    const uintptr_t TextBeginAddress = reinterpret_cast<uintptr_t>(Text.Begin);
    const uintptr_t TextEndAddress = TextBeginAddress + Text.Size;

    std::vector<uintptr_t> TagSites;

    /* The section can contain unreadable pages, so the tag is only searched inside the readable spans. */
    ForEachReadableRange(TextBeginAddress, Text.Size, [&TagSites](const uintptr_t RangeStart, const uintptr_t RangeSize) -> void
    {
        const uint8* Cursor = reinterpret_cast<const uint8*>(RangeStart);
        const uint8* const RangeEnd = Cursor + RangeSize;

        while ((Cursor + sizeof(PackageFileTagBytes)) <= RangeEnd)
        {
            Cursor = static_cast<const uint8*>(memchr(Cursor, PackageFileTagBytes[0x0], static_cast<size_t>(RangeEnd - Cursor)));

            if (Cursor == nullptr)
                break;

            /* 'memchr' can land on the last few bytes, where the full tag would run past the readable span. */
            if ((Cursor + sizeof(PackageFileTagBytes)) <= RangeEnd && memcmp(Cursor, PackageFileTagBytes, sizeof(PackageFileTagBytes)) == 0)
                TagSites.push_back(reinterpret_cast<uintptr_t>(Cursor));

            ++Cursor;
        }
    });

    if (TagSites.empty())
    {
        std::cerr << "MappingGeneration: 'PACKAGE_FILE_TAG' was not found, the file version cannot be scanned\n";
        return false;
    }

    std::unordered_map<uintptr_t, int32> Votes;

    for (const uintptr_t TagSite : TagSites)
    {
        const uintptr_t WindowStart = (TagSite > TextBeginAddress + PackageFileTagScanWindow) ? (TagSite - PackageFileTagScanWindow) : TextBeginAddress;
        const uintptr_t WindowEnd = (std::min)(TagSite + PackageFileTagScanWindow, TextEndAddress - sizeof(int32));

        if (WindowEnd <= WindowStart)
            continue;

        ForEachReadableRange(WindowStart, WindowEnd - WindowStart, [&Votes, bIsUE5, TextBeginAddress, TextEndAddress](const uintptr_t RangeStart, const uintptr_t RangeSize) -> void
        {
            const uintptr_t RangeEnd = RangeStart + RangeSize;

            for (uintptr_t Address = RangeStart; (Address + sizeof(int32)) <= RangeEnd; ++Address)
            {
                const int32 Displacement = *reinterpret_cast<const int32*>(Address);
                const uintptr_t Target = Address + sizeof(int32) + static_cast<int64_t>(Displacement);

                /* The constant is in a data section, so a target inside '.text' is a coincidence. */
                if (Target >= TextBeginAddress && Target < TextEndAddress)
                    continue;

                int32 FileVersionUE4 = 0x0;
                int32 FileVersionUE5 = 0x0;

                if (!TryReadPackageFileVersion(Target, bIsUE5, FileVersionUE4, FileVersionUE5))
                    continue;

                ++Votes[Target];
            }
        });
    }

    if (Votes.empty())
    {
        std::cerr << "MappingGeneration: No 'FPackageFileVersion' was referenced near a 'PACKAGE_FILE_TAG' use\n";
        return false;
    }

    /* A tie means the evidence is not conclusive, so nothing is written rather than a guess. */
    const auto Best = std::ranges::max_element(Votes, [](const auto& Left, const auto& Right)
    {
        return Left.second < Right.second;
    });

    const int32 Winners = static_cast<int32>(std::ranges::count_if(Votes, [&Best](const auto& Entry)
    {
        return Entry.second == Best->second;
    }));

    if (Winners != 0x1)
    {
        std::cerr << std::format("MappingGeneration: The file version scan was inconclusive, {} targets tied with {} vote(s)\n", Winners, Best->second);
        return false;
    }

    if (!TryReadPackageFileVersion(Best->first, bIsUE5, OutFileVersionUE4, OutFileVersionUE5))
        return false;

    std::cerr << std::format("MappingGeneration: Scanned 'GPackageFileUEVersion' at 0x{:X} with {} vote(s)\n", Best->first, Best->second);

    return true;
}

void* EngineVersioning::FindNetworkCompatibleChangelistByScan()
{
    /*
    * 'FNetworkVersion::GetNetworkCompatibleChangelist()' is only exported by editor builds, and unlike the
    * file version it is a function, so it has to be located rather than read. 'FApp' logs its result, which
    * gives an exact anchor: the call feeding that log line is the function itself.
    */
    /* Only a direct 'lea' onto the literal is accepted, an indirect one could match something else first. */
    const void* LogStringReference = Platform::FindByStringInAllSections<false, wchar_t>(L"Net CL: %u", 0x0, 0x0, Settings::General::bSearchOnlyExecutableSectionsForStrings);

    if (LogStringReference == nullptr)
    {
        std::cerr << "MappingGeneration: The 'Net CL: %u' log string was not found, assuming NetCL = 0\n";
        return nullptr;
    }

    /* The log arguments are set up between the call and the string reference, so walk back to the nearest call. */
    const auto Reference = static_cast<const uint8*>(LogStringReference);

    for (uintptr_t Back = 0x1; Back <= NetworkChangelistBackwardScanRange; ++Back)
    {
        const uintptr_t Candidate = reinterpret_cast<uintptr_t>(Reference) - Back;

        /* The back-scan can cross into a page the protector left unreadable. */
        if (!IsSpanReadable(Candidate, RelativeCallInstructionSize))
            continue;

        if (*reinterpret_cast<const uint8*>(Candidate) != RelativeCallOpcode)
            continue;

        const int32 Relative = *reinterpret_cast<const int32*>(Candidate + 0x1);
        const uintptr_t Target = Candidate + 0x5 + static_cast<int64_t>(Relative);

        if (!Platform::IsAddressInProcessRange(Target))
            return nullptr;

        return reinterpret_cast<void*>(Target);
    }

    std::cerr << "MappingGeneration: No call was found above the 'Net CL: %u' log string, assuming NetCL = 0\n";

    return nullptr;
}

void EngineVersioning::WritePackageVersioning(StreamType& InUsmap, const bool bWriteEngineVersion)
{
    const FEngineVersion EngineVersion = ParseEngineVersion();

    /*
    * Without a resolvable engine version there is nothing to version against, so the whole block is left out.
    * The engine version string is only resolvable when this dumper could read it from the game (it may be a user override).
    */
    const bool bHasVersioning = EngineVersion.IsValid();
    WriteToStream(InUsmap, static_cast<int32>(bHasVersioning));

    std::cerr << std::format("MappingGeneration: bHasVersioning = {} (GameVersion = \"{}\")\n", bHasVersioning, Settings::Generator::GameVersion);

    if (!bHasVersioning)
    {
        std::cerr << "MappingGeneration: Skipping the package versioning block, no engine version could be parsed\n";
        return;
    }

    if (bWriteEngineVersion)
    {
        WriteToStream(InUsmap, EngineVersion.Major);
        WriteToStream(InUsmap, EngineVersion.Minor);
        WriteToStream(InUsmap, EngineVersion.Patch);
        WriteToStream(InUsmap, EngineVersion.Changelist);
        WriteUEFString(InUsmap, EngineVersion.Branch);

        std::cerr << std::format("MappingGeneration: EngineVersion = {}.{}.{}-{}+{}\n",
                                 EngineVersion.Major, EngineVersion.Minor, EngineVersion.Patch, EngineVersion.Changelist, EngineVersion.Branch);
    }

    /* Recover the package version, the custom versions and the network changelist from the live process. */
    const auto [FileVersionUE4, FileVersionUE5, CustomVersions, NetCL] = ReadRuntimeVersioning(EngineVersion.Major);

    WriteToStream(InUsmap, FileVersionUE4);
    WriteToStream(InUsmap, FileVersionUE5);

    WriteToStream(InUsmap, static_cast<uint32>(CustomVersions.size()));
    for (const auto& [Key, Version] : CustomVersions)
    {
        InUsmap.write(reinterpret_cast<const char*>(Key), FCustomVersionKeySize);
        WriteToStream(InUsmap, Version);
    }

    WriteToStream(InUsmap, NetCL);

    std::cerr << std::format("MappingGeneration: Writing {} custom version(s), NetCL = {}\n", CustomVersions.size(), NetCL);
}
