#pragma once
#include <cassert>

#include "Enums.h"

static constexpr uint8_t FiveBitPermutation[32] = {
    0x20, 0x1D, 0x08, 0x18, 0x06, 0x0F, 0x15, 0x19,
    0x13, 0x1F, 0x17, 0x10, 0x14, 0x0C, 0x0E, 0x02,
    0x16, 0x11, 0x12, 0x0A, 0x0B, 0x1E, 0x04, 0x07,
    0x1B, 0x03, 0x0D, 0x1A, 0x1C, 0x01, 0x09, 0x05,
};

inline uint8 SmallPearsonHash(const char* StringToHash)
{
    uint8 Hash = 0;

    while(*StringToHash != '\0')
    {
        const uint8 MaskedDownChar = (*StringToHash - 'A') & 0b11111;
        Hash = FiveBitPermutation[Hash ^ MaskedDownChar];
        StringToHash++;
    }

    return (Hash & 0b11111);
}

using StringTableIndex = int32;

#pragma pack(0x1)
class StringEntry
{
public:
    static constexpr int32 MaxStringLength = 512;

    static constexpr int32 StringEntrySizeWithoutStr = 0x3;

private:
    friend class StringTable;
    friend class StringTableTest;

private:
    // Length of object name
    uint16 Length : 9;

    // If this string uses char or wchar_t
    uint16 bIsWide : 1;

    // If this name is unique amongst others --- always true if RawNameLength > 0
    mutable uint16 bIsUnique : 1;

    // PearsonHash reduced to 5 bits --- only computed if string was added when StringTable::CurrentMode == EDuplicationCheckingMode::Check
    uint16 Hash : 5;

    // Length of RawName to be appended to 'Legth' if this is a FullName, zero if this name is edited (eg. "UEditTool3Plus3")
    uint8 RawNameLength;

    union
    {
        // null-terminated
        const char Char[MaxStringLength];
        const wchar_t WChar[MaxStringLength];
    };

private:
    inline int32 GetLengthBytes() const { return StringEntrySizeWithoutStr + Length + RawNameLength; }

public:
    inline bool IsUnique() const { return bIsUnique; }
    inline bool IsPrefixedValidName() const { return RawNameLength == 0; }

    inline std::string_view GetRawName() const { return Char + Length; }
    inline std::wstring_view GetWideRawName() const { return WChar + Length; }

    inline std::string_view GetUniqueName() const { return Char; }
    inline std::wstring_view GetWideUniqueName() const { return WChar; }

    inline std::string_view GetFullName() const { return Char; }
    inline std::wstring_view GetWideFullName() const { return WChar; }
};

struct HashStringTableIndex
{
    int32 bIsChecked : 1;
    int32 HashIndex : 5;
    int32 InBlockOffset : 26;

    static inline HashStringTableIndex FromInt(int32 Idx)
    {
        return *reinterpret_cast<HashStringTableIndex*>(Idx);
    }

    explicit inline operator int32()
    {
        return *reinterpret_cast<int32*>(this);
    }
};

class HashStringTable
{
private:
    static constexpr int32 NumHashBits = 5;
    static constexpr int32 NumBuckets = 1 << NumHashBits;

    /* Checked, Unchecked */
    static constexpr int32 NumSectionsPerBucket = 2;

private:
    struct Bucket
    {
        // One allocated block, split in two sections, checked and unchecked
        uint8* Data;
        int32 UncheckedSize;
        int32 CheckedSize;
        int32 UncheckedSizeMax;
        int32 CheckedSizeMax;
    };

private:
    Bucket Buckets[NumBuckets];

public:
    HashStringTable() = default;

    HashStringTable(int32 PerBucketPerSectionSize)
    {
        if (PerBucketPerSectionSize <= 0x0)
            return;

        for (int i = 0; i < NumBuckets; i++)
        {
            Bucket& CurrentBucket = Buckets[i];
            CurrentBucket.Data = static_cast<uint8*>(malloc(PerBucketPerSectionSize * NumSectionsPerBucket));

            CurrentBucket.UncheckedSize = 0x0;
            CurrentBucket.CheckedSize = 0x0;

            CurrentBucket.UncheckedSizeMax = PerBucketPerSectionSize;
            CurrentBucket.CheckedSizeMax = PerBucketPerSectionSize;
        }
    }

private:
    inline bool CanFit(const Bucket& Bucket, int32 StrLengthBytes, bool bIsChecked) const
    {
        const int32 EntryLength = StringEntry::StringEntrySizeWithoutStr + StrLengthBytes;

        if (bIsChecked)
        {
            return (EntryLength + Bucket.CheckedSize) <= Bucket.CheckedSizeMax;
        }

        return (EntryLength + Bucket.UncheckedSize) <= Bucket.UncheckedSizeMax;
    }

    inline StringEntry& GetRefToEmpty(const Bucket& Bucket, bool bIsChecked)
    {
        if (bIsChecked)
            return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSizeMax + Bucket.CheckedSize);

        return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSize);
    }

    inline void ResizeBucket(Bucket& Bucket, bool bIsChecked)
    {
        if (bIsChecked)
        {
            /* Only extend the checked part of the bucket */
            int32 NewBucketSize = Bucket.UncheckedSizeMax + (Bucket.CheckedSizeMax * 2);

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, NewBucketSize));

            if (NewData)
                Bucket.Data = NewData;
        }
        else
        {
            int32 OldBucketUncheckedSizeMax = Bucket.UncheckedSizeMax;
            int32 NewBucketUncheckedSizeMax = Bucket.UncheckedSizeMax * 2;

            /* Only extend the unchecked part of the bucket */

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, Bucket.CheckedSizeMax + NewBucketUncheckedSizeMax));

            if (NewData)
            {
                /* Move checked part of the bucket to the back so we don't write over it when adding new unchecked elements */
                Bucket.Data = NewData;
                memmove(Bucket.Data + OldBucketUncheckedSizeMax, Bucket.Data + NewBucketUncheckedSizeMax, Bucket.CheckedSize);
            }
        }
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> AddUnchecked(const CharType* Str, int32 Length, int32 RawNameLength, uint8 Hash, bool bIsChecked)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = (Length + RawNameLength) * sizeof(CharType);

        Bucket& Bucket = Buckets[Hash];


        StringEntry& NewEmpty = GetRefToEmpty();

        NewEmpty.Length = Length;
        NewEmpty.RawNameLength = RawNameLength;
        NewEmpty.bIsWide = std::is_same_v<CharType, wchar_t>;
        NewEmpty.Hash = Hash;

        // Initially always true, later marked as false if duplicate is found
        NewEmpty.bIsUnique = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy((void*)(NewEmpty.WChar), Str, LengthBytes);

        const int32 LengthOfNewEntry = StringEntry::StringEntrySizeWithoutStr + LengthBytes;

        HashStringTableIndex ReturnIndex;
        ReturnIndex.bIsChecked = bIsChecked;
        ReturnIndex.HashIndex = Hash;


        return { ReturnIndex, true };
    }

public:

};

class StringTable
{
private:
    friend class StringTableTest;

private:
    enum class EDuplicationCheckingMode : int32
    {
        Check,
        DontCheck,
        None
    };

    struct StringTableAddResult
    {
        StringTableIndex Index;
        bool bAddedNew;

        operator StringTableIndex() const { return Index; }
        operator bool() const { return bAddedNew; }
    };

private:
    uint8* Data;
    int32 UncheckedSize;
    int32 CheckedSize;
    int32 AllocationSize;
    EDuplicationCheckingMode CurrentMode;

public:
    StringTable()
        : Data(nullptr)
        , UncheckedSize(0)
        , CheckedSize(0)
        , AllocationSize(0)
        , CurrentMode(EDuplicationCheckingMode::None)
    {
    }

    StringTable(int32 Size)
        : Data(reinterpret_cast<uint8*>(malloc(Size > 0x1000 ? Size : 0x1000)))
        , UncheckedSize(0)
        , CheckedSize(0)
        , AllocationSize(Size > 0x1000 ? Size : 0x1000)
        , CurrentMode(EDuplicationCheckingMode::DontCheck)
    {
    }

    ~StringTable()
    {
        if (Data)
        {
            free(Data);
            Data = nullptr;
        }
    }

public:
    StringTable(StringTable&&) = delete;
    StringTable(const StringTable&) = delete;

    StringTable& operator=(StringTable&&) = delete;
    StringTable& operator=(const StringTable&) = delete;

public:
    class StringTableIterator
    {
    private:
        const StringTable& IteratedTable;
        int32 Pos;

    public:
        StringTableIterator(const StringTable& Table, int32 StartPos = 0)
            : IteratedTable(Table)
            , Pos(StartPos)
        {
        }

    public:
        inline int32 GetPos() const { return Pos; }

    public:
        inline bool operator==(const StringTableIterator& Other) const { return Pos == Other.Pos; }
        inline bool operator!=(const StringTableIterator& Other) const { return Pos != Other.Pos; }

        inline const StringEntry& operator*() const { return IteratedTable.GetStringEntry(Pos); }

        inline StringTableIterator& operator++()
        {
            const StringEntry& Current = IteratedTable.GetStringEntry(Pos);
            Pos += Current.GetLengthBytes();

            return *this;
        }
    };

public:
    inline StringTableIterator begin() const { return StringTableIterator(*this, 0); }
    inline StringTableIterator end()   const { return StringTableIterator(*this, GetCurrentSize()); }

private:
    inline bool CanFit(int32 StrLengthBytes) const
    {
        return (StringEntry::StringEntrySizeWithoutStr + StrLengthBytes + UncheckedSize + CheckedSize) <= AllocationSize;
    }

    inline bool IsValid() const
    {
        return Data != nullptr;
    }

    inline void Allocate(int32 Size)
    {
        constexpr int32 MinSize = 0x1000;

        Data = reinterpret_cast<uint8*>(malloc(Size > MinSize ? Size : MinSize));
        UncheckedSize = 0;
        CheckedSize = 0;
        AllocationSize = Size > MinSize ? Size : MinSize;
        CurrentMode = EDuplicationCheckingMode::DontCheck;
    }

    inline void Resize(int32 NewSize)
    {
        uint8* NewData = reinterpret_cast<uint8*>(realloc(Data, NewSize));

        assert(NewData != nullptr);

        Data = NewData ? NewData : Data;
        AllocationSize = NewSize;
    }

    inline StringEntry& GetRefToEmpty()
    {
        return *reinterpret_cast<StringEntry*>(Data + UncheckedSize + CheckedSize);
    }

    template<typename CharType>
    inline StringTableAddResult AddUnchecked(const CharType* Str, int32 Length, int32 RawNameLength, uint8 Hash)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = (Length + RawNameLength) * sizeof(CharType);

        if (!CanFit(StringEntry::StringEntrySizeWithoutStr + LengthBytes))
            Resize(AllocationSize * 1.5f);

        StringEntry& NewEmpty = GetRefToEmpty();

        NewEmpty.Length = Length;
        NewEmpty.RawNameLength = RawNameLength;
        NewEmpty.bIsWide = std::is_same_v<CharType, wchar_t>;
        NewEmpty.Hash = Hash;

        // Initially always true, later marked as false if duplicate is found
        NewEmpty.bIsUnique = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy((void*)(NewEmpty.WChar), Str, LengthBytes);

        const int32 LengthOfNewEntry = StringEntry::StringEntrySizeWithoutStr + LengthBytes;

        const int32 ReturnIndex = GetCurrentSize();

        (CurrentMode == EDuplicationCheckingMode::DontCheck ? UncheckedSize : CheckedSize) += LengthOfNewEntry;

        return { ReturnIndex, true };
    }

    template<typename CharType>
    static inline int32 Strcmp(const CharType* String, const StringEntry& Entry)
    {
        if constexpr (std::is_same_v<CharType, char>)
        {
            return strcmp(Entry.Char, String);
        }
        else
        {
            return wcscmp(Entry.WChar, String);
        }
    }

public:
    inline void SwitchToCheckedMode()
    {
        CurrentMode = EDuplicationCheckingMode::Check;
    }

    inline int32 GetCurrentSize() const
    {
        return UncheckedSize + CheckedSize;
    }

    inline const StringEntry& GetStringEntry(StringTableIndex EntryIndex) const
    {
        assert(EntryIndex < GetCurrentSize() && "Index out of bounds!");

        return *reinterpret_cast<StringEntry*>(Data + EntryIndex);
    }

    template<typename CharType>
    inline StringTableAddResult FindOrAdd(const CharType* Str, int32 Length, int32 RawNameLength = 0)
    {
        const int CombinedLength = Length + RawNameLength;
        const bool bIsWChar = std::is_same_v<CharType, wchar_t>;

        if (!Str || CombinedLength <= 0 || CombinedLength > StringEntry::MaxStringLength)
        {
            std::cout << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "LengthBytes <= 0" : "LengthBytes > MaxStringLength") << std::endl;
            return { -1, false };
        }

        if (CurrentMode == EDuplicationCheckingMode::DontCheck)
            return AddUnchecked(Str, Length, RawNameLength, 0x0);

        uint8 Hash = 0x0;
        
        if constexpr (!bIsWChar)
            Hash = SmallPearsonHash(Str);

        for (auto It = StringTableIterator(*this, UncheckedSize); It != this->end(); ++It)
        {
            const StringEntry& Entry = *It;

            if (Entry.bIsWide == bIsWChar && Entry.Length == Length && Entry.Hash == Hash && Strcmp(Str, Entry) == 0)
            {
                Entry.bIsUnique = false;
                return { It.GetPos(), false };
            }
        }

        // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
        return AddUnchecked(Str, Length, RawNameLength, Hash);
    }

    inline StringTableAddResult FindOrAdd(const std::string& String)
    {
        size_t LastDotIdx = String.find_last_of('.');

        if (LastDotIdx == -1) {
            return FindOrAdd(String.c_str(), String.size() + 1, 0);
        }
        else {
            return FindOrAdd(String.c_str(), LastDotIdx + 1, String.size() - LastDotIdx);
        }
    }

public: /* DEBUG */
    inline uintptr_t DEBUGGetDataPtr() const
    {
        return reinterpret_cast<uintptr_t>(Data);
    }
};