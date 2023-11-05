#pragma once
#include "Enums.h"

#include <cassert>
#include <format>
#include <iostream>

inline constexpr uint32 NumHashBits = 5;
inline constexpr uint32 MaxHashNumber = 1 << NumHashBits;
inline constexpr uint32 HashMask = MaxHashNumber - 1;

static constexpr uint8_t FiveBitPermutation[MaxHashNumber] = {
    0x20, 0x1D, 0x08, 0x18, 0x06, 0x0F, 0x15, 0x19,
    0x13, 0x1F, 0x17, 0x10, 0x14, 0x0C, 0x0E, 0x02,
    0x16, 0x11, 0x12, 0x0A, 0x0B, 0x1E, 0x04, 0x07,
    0x1B, 0x03, 0x0D, 0x1A, 0x1C, 0x01, 0x09, 0x05,
};

inline uint8 SmallPearsonHash(const char* StringToHash)
{
    uint8 Hash = 0;

    while (*StringToHash != '\0')
    {
        const uint8 MaskedDownChar = (*StringToHash - 'A') & HashMask;
        Hash = FiveBitPermutation[Hash ^ MaskedDownChar];
        StringToHash++;
    }

    return (Hash & HashMask);
}


class StringEntry
{
private:
    friend class HashStringTable;
    friend class HashStringTableTest;

    template<typename CharType>
    friend int32 Strcmp(const CharType* String, const StringEntry& Entry);

public:
    static constexpr int32 MaxStringLength = 1 << 11;

    static constexpr int32 StringEntrySizeWithoutStr = 0x4;

private:
    // Length of object name
    uint32 Length : 11;

    // Length of RawName to be appended to 'Legth' if this is a FullName, zero if this name is edited (eg. "UEditTool3Plus3")
    uint32 RawNameLength : 10;

    // If this string uses char or wchar_t
    uint32 bIsWide : 1;

    // If this name is unique amongst others --- always true if RawNameLength > 0
    mutable uint32 bIsUnique : 1;

    // PearsonHash reduced to 5 bits --- only computed if string was added when StringTable::CurrentMode == EDuplicationCheckingMode::Check
    uint32 Hash : 5;

    // Unused bits
    uint32 Unused : 4;

    union
    {
        // NOT null-terminated
        char Char[MaxStringLength];
        wchar_t WChar[MaxStringLength];
    };

private:
    inline int32 GetLengthBytes() const { return StringEntrySizeWithoutStr + Length + RawNameLength; }

    inline int32 GetStringLength() const { return Length + RawNameLength; }

public:
    inline bool IsUnique() const { return bIsUnique; }
    inline bool IsPrefixedValidName() const { return RawNameLength == 0; }

    inline uint8 GetHash() const { return Hash; }

    inline std::string GetRawName() const { return std::string(Char + Length, RawNameLength); }
    inline std::wstring GetWideRawName() const { return std::wstring(WChar + Length, RawNameLength); }
    inline std::string_view GetRawNameView() const { return std::string_view(Char + Length, RawNameLength); }
    inline std::wstring_view GetWideRawNameView() const { return std::wstring_view(WChar + Length, RawNameLength); }

    inline std::string GetUniqueName() const { return std::string(Char, GetStringLength()); }
    inline std::wstring GetWideUniqueName() const { return std::wstring(WChar, GetStringLength()); }
    inline std::string_view GetUniqueNameView() const { return std::string_view(Char, GetStringLength()); }
    inline std::wstring_view GetWideUniqueNameView() const { return std::wstring_view(WChar, GetStringLength()); }

    inline std::string GetFullName() const { return std::string(Char, GetStringLength()); }
    inline std::wstring GetWideFullName() const { return std::wstring(WChar, GetStringLength()); }
    inline std::string_view GetFullNameView() const { return std::string_view(Char, GetStringLength()); }
    inline std::wstring_view GetWideFullNameView() const { return std::wstring_view(WChar, GetStringLength()); }
};

template<typename CharType>
inline int32 Strcmp(const CharType* String, const StringEntry& Entry)
{
    static_assert(std::is_same_v<CharType, wchar_t> || std::is_same_v<CharType, char>);

    return memcmp(Entry.Char, String, Entry.GetStringLength() * sizeof(CharType));
}

struct HashStringTableIndex
{
public:
    static inline uint32 InvalidIndex = -1;

public:
    uint32 bIsChecked : 1;
    uint32 HashIndex : 5;
    uint32 InBucketOffset : 26;

public:
    inline HashStringTableIndex& operator=(uint32 Value)
    {
        *reinterpret_cast<uint32*>(this) = Value;

        return *this;
    }

    static inline HashStringTableIndex FromInt(uint32 Idx)
    {
        return *reinterpret_cast<HashStringTableIndex*>(&Idx);
    }

    inline operator uint32() const
    {
        return *reinterpret_cast<const int32*>(this);
    }

    explicit inline operator bool() const
    {
        return *this != InvalidIndex;
    }

    inline bool operator==(HashStringTableIndex Other) const { return static_cast<uint32>(*this) == static_cast<uint32>(Other); }
    inline bool operator!=(HashStringTableIndex Other) const { return static_cast<uint32>(*this) != static_cast<uint32>(Other); }
};

class HashStringTable
{
private:
    static constexpr int64 NumHashBits = 5;
    static constexpr int64 NumBuckets = 1 << NumHashBits;

    /* Checked, Unchecked */
    static constexpr int64 NumSectionsPerBucket = 2;

private:
    struct StringBucket
    {
        // One allocated block, split in two sections, checked and unchecked
        uint8* Data;
        uint32 UncheckedSize;
        uint32 CheckedSize;
        uint32 UncheckedSizeMax;
        uint32 CheckedSizeMax;
    };

private:
    StringBucket Buckets[NumBuckets];

public:
    HashStringTable(uint32 BucketUncheckedSize = 0x5000, uint32 BucketCheckedSize = 0x5000)
    {
        assert((BucketUncheckedSize + BucketCheckedSize) >= 0x0 && "HashStringTable(0x0, 0x0) is invalid!");

        for (int i = 0; i < NumBuckets; i++)
        {
            StringBucket& CurrentBucket = Buckets[i];
            CurrentBucket.Data = static_cast<uint8*>(malloc(BucketUncheckedSize + BucketCheckedSize));

            CurrentBucket.UncheckedSize = 0x0;
            CurrentBucket.CheckedSize = 0x0;

            CurrentBucket.UncheckedSizeMax = BucketUncheckedSize;
            CurrentBucket.CheckedSizeMax = BucketCheckedSize;
        }
    }

    ~HashStringTable()
    {
        for (int i = 0; i < NumBuckets; i++)
        {
            StringBucket& CurrentBucket = Buckets[i];

            if (!CurrentBucket.Data)
                continue;

            free(CurrentBucket.Data);
            CurrentBucket.Data = nullptr;
        }
    }

public:
    class HashBucketIterator
    {
    private:
        const StringBucket* IteratedBucket;
        uint32 InBucketIndex;

    public:
        HashBucketIterator(const StringBucket& Bucket, uint32 InBucketStartPos = 0)
            : IteratedBucket(&Bucket)
            , InBucketIndex(InBucketStartPos)
        {
            if (InBucketIndex >= Bucket.UncheckedSize && InBucketIndex < Bucket.UncheckedSizeMax)
                InBucketIndex = Bucket.UncheckedSizeMax;
        }

    public:
        static inline HashBucketIterator begin(const StringBucket& Bucket) { return HashBucketIterator(Bucket, 0); }
        static inline HashBucketIterator beginChecked(const StringBucket& Bucket) { return HashBucketIterator(Bucket, Bucket.UncheckedSizeMax); }
        static inline HashBucketIterator end(const StringBucket& Bucket) { return HashBucketIterator(Bucket, Bucket.CheckedSize != 0 ? Bucket.UncheckedSizeMax + Bucket.CheckedSize : Bucket.UncheckedSize); }

    public:
        inline uint32 GetInBucketIndex() const { return InBucketIndex; }
        inline const StringEntry& GetStringEntry() const { return *reinterpret_cast<StringEntry*>(IteratedBucket->Data + InBucketIndex); }

    public:
        inline bool operator==(const HashBucketIterator& Other) const { return InBucketIndex == Other.InBucketIndex; }
        inline bool operator!=(const HashBucketIterator& Other) const { return InBucketIndex != Other.InBucketIndex; }

        inline const StringEntry& operator*() const { return GetStringEntry(); }

        inline HashBucketIterator& operator++()
        {
            InBucketIndex += GetStringEntry().GetLengthBytes();

            const bool bIsBetweenUncheckedAndCheckedRegion = InBucketIndex >= IteratedBucket->UncheckedSize && InBucketIndex < IteratedBucket->UncheckedSizeMax;

            if (bIsBetweenUncheckedAndCheckedRegion)
                InBucketIndex = IteratedBucket->UncheckedSizeMax; // continue to next valid memory

            return *this;
        }
    };

    class HashStringTableIterator
    {
    private:
        const HashStringTable& IteratedTable;
        HashBucketIterator CurrentBucketIterator;
        uint32 BucketIdx;

    public:
        HashStringTableIterator(const HashStringTable& Table, uint32 BucketStartPos = 0, uint32 InBucketStartPos = 0)
            : IteratedTable(Table)
            , CurrentBucketIterator(Table.Buckets[BucketStartPos], InBucketStartPos)
            , BucketIdx(BucketStartPos)
        {
            while (CurrentBucketIterator == HashBucketIterator::end(IteratedTable.Buckets[BucketIdx]) && (++BucketIdx < NumBuckets))
                CurrentBucketIterator = HashBucketIterator::begin(IteratedTable.Buckets[BucketIdx]);
        }

        /* Only used for 'end' */
        HashStringTableIterator(const HashStringTable& Table, HashBucketIterator LastBucketEndIterator)
            : IteratedTable(Table)
            , CurrentBucketIterator(LastBucketEndIterator)
            , BucketIdx(NumBuckets)
        {
        }

    public:
        inline uint32 GetBucketIndex() const { return BucketIdx; }
        inline uint32 GetInBucketIndex() const { return CurrentBucketIterator.GetInBucketIndex(); }

    public:
        inline bool operator==(const HashStringTableIterator& Other) const { return BucketIdx == Other.BucketIdx && CurrentBucketIterator == Other.CurrentBucketIterator; }
        inline bool operator!=(const HashStringTableIterator& Other) const { return BucketIdx != Other.BucketIdx || CurrentBucketIterator != Other.CurrentBucketIterator; }

        inline const StringEntry& operator*() const { return *CurrentBucketIterator; }

        inline HashStringTableIterator& operator++()
        {
            ++CurrentBucketIterator;

            while (CurrentBucketIterator == HashBucketIterator::end(IteratedTable.Buckets[BucketIdx]) && (++BucketIdx < NumBuckets))
                CurrentBucketIterator = HashBucketIterator::begin(IteratedTable.Buckets[BucketIdx]);

            return *this;
        }
    };

public:
    inline HashStringTableIterator begin() const { return HashStringTableIterator(*this, 0); }
    inline HashStringTableIterator end()   const { return HashStringTableIterator(*this, HashBucketIterator::end(Buckets[NumBuckets - 1])); }

private:
    inline bool CanFit(const StringBucket& Bucket, int32 StrLengthBytes, bool bIsChecked) const
    {
        const int32 EntryLength = StringEntry::StringEntrySizeWithoutStr + StrLengthBytes;

        if (bIsChecked)
            return (Bucket.CheckedSize + EntryLength) <= Bucket.CheckedSizeMax;

        return (Bucket.UncheckedSize + EntryLength) <= Bucket.UncheckedSizeMax;
    }

    inline StringEntry& GetRefToEmpty(const StringBucket& Bucket, bool bIsChecked)
    {
        if (bIsChecked)
            return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSizeMax + Bucket.CheckedSize);

        return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSize);
    }

    inline const StringEntry& GetStringEntry(const StringBucket& Bucket, int32 InBucketIndex) const
    {
        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline const StringEntry& GetStringEntry(int32 BucketIndex, int32 InBucketIndex) const
    {
        assert(BucketIndex < 32 && "Bucket index out of range!");

        const StringBucket& Bucket = Buckets[BucketIndex];

        assert(InBucketIndex > 0 && InBucketIndex < (Bucket.UncheckedSize + Bucket.CheckedSize) && "InBucketIndex was out of range!");

        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline void ResizeBucket(StringBucket& Bucket, bool bIsChecked)
    {
        int32 BucketIdx = &Bucket - Buckets;

        if (bIsChecked)
        {
            /* Only extend the checked part of the bucket */
            const uint32 OldCheckedSizeMax = Bucket.CheckedSizeMax;
            const uint64 NewCheckedSizeMax = Bucket.CheckedSizeMax * 1.5;

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, Bucket.UncheckedSizeMax + NewCheckedSizeMax));

            if (NewData)
            {
                Bucket.CheckedSizeMax = NewCheckedSizeMax;
                Bucket.Data = NewData;
            }
        }
        else
        {
            /* Only extend the unchecked part of the bucket */
            const uint32 OldBucketUncheckedSizeMax = Bucket.UncheckedSizeMax;
            const uint64 NewBucketUncheckedSizeMax = Bucket.UncheckedSizeMax * 1.5;

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, Bucket.CheckedSizeMax + NewBucketUncheckedSizeMax));

            if (NewData)
            {
                /* Move checked part of the bucket to the back so we don't write over it when adding new unchecked elements */
                Bucket.Data = NewData;
                Bucket.UncheckedSizeMax = NewBucketUncheckedSizeMax;

                if (Bucket.CheckedSize > 0)
                    memmove(Bucket.Data + NewBucketUncheckedSizeMax, Bucket.Data + OldBucketUncheckedSizeMax, Bucket.CheckedSize);
            }
        }
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> AddUnchecked(const CharType* Str, int32 Length, int32 RawNameLength, uint8 Hash, bool bIsChecked)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = (Length + RawNameLength) * sizeof(CharType);

        StringBucket& Bucket = Buckets[Hash];

        if (!CanFit(Bucket, LengthBytes, bIsChecked))
            ResizeBucket(Bucket, bIsChecked);

        StringEntry& NewEmptyEntry = GetRefToEmpty(Bucket, bIsChecked);

        NewEmptyEntry.Length = Length;
        NewEmptyEntry.RawNameLength = RawNameLength;
        NewEmptyEntry.bIsWide = std::is_same_v<CharType, wchar_t>;
        NewEmptyEntry.Hash = Hash;

        // Initially always true, later marked as false if duplicate is found
        NewEmptyEntry.bIsUnique = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy(NewEmptyEntry.WChar, Str, LengthBytes);

        HashStringTableIndex ReturnIndex;
        ReturnIndex.bIsChecked = bIsChecked;
        ReturnIndex.HashIndex = Hash;
        ReturnIndex.InBucketOffset = bIsChecked ? Bucket.CheckedSize : Bucket.UncheckedSize;

        (bIsChecked ? Bucket.CheckedSize : Bucket.UncheckedSize) += NewEmptyEntry.GetLengthBytes();

        return { ReturnIndex, true };
    }

public:
    inline const StringEntry& operator[](HashStringTableIndex Index) const
    {
        return GetStringEntry(Index);
    }

public:
    inline const StringBucket& GetBucket(uint32 Index) const
    {
        assert(Index < NumBuckets && "Index out of range!");

        return Buckets[Index];
    }

    inline const StringEntry& GetStringEntry(HashStringTableIndex Index) const
    {
        assert(Index.HashIndex < 32 && "Bucket index out of range!");

        const StringBucket& Bucket = Buckets[Index.HashIndex];

        assert(Index.InBucketOffset >= 0 && Index.InBucketOffset < (Bucket.UncheckedSizeMax + Bucket.CheckedSize) && "InBucketIndex was out of range!");

        if (Index.bIsChecked)
            return  *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSizeMax + Index.InBucketOffset);

        return *reinterpret_cast<StringEntry*>(Bucket.Data + Index.InBucketOffset);
    }

    template<typename CharType>
    inline HashStringTableIndex Find(const CharType* Str, int32 Length, uint8 Hash, bool bIsChecked, int32 RawNameLength = 0)
    {
        constexpr bool bIsWchar = std::is_same_v<CharType, wchar_t>;

        StringBucket& Bucket = Buckets[Hash];

        /* Try to find duplications withing 'checked' regions */
        for (auto It = HashBucketIterator::beginChecked(Bucket); It != HashBucketIterator::end(Bucket); ++It)
        {
            const StringEntry& Entry = *It;

            if (Entry.Length == Length && Entry.bIsWide == bIsWchar && Strcmp(Str, Entry) == 0)
            {
                HashStringTableIndex Idx;
                Idx.bIsChecked = true;
                Idx.HashIndex = Hash;
                Idx.InBucketOffset = It.GetInBucketIndex();

                return Idx;
            }
        }

        return HashStringTableIndex::FromInt(-1);
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const CharType* Str, int32 Length, bool bIsChecked, int32 RawNameLength = 0)
    {
        const int CombinedLength = Length + RawNameLength;
        constexpr bool bIsWChar = std::is_same_v<CharType, wchar_t>;

        static_assert(!bIsWChar, "'wchar_t' is not supported by the hashing function yet!");

        if (!Str || CombinedLength <= 0 || CombinedLength > StringEntry::MaxStringLength)
        {
            std::cout << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "LengthBytes <= 0" : "LengthBytes > MaxStringLength") << std::endl;
            return { HashStringTableIndex(-1), false };
        }

        uint8 Hash = SmallPearsonHash(Str);

        if (!bIsChecked)
            return AddUnchecked(Str, Length, RawNameLength, Hash, bIsChecked);

        HashStringTableIndex ExistingIndex = Find(Str, Length, Hash, bIsChecked, RawNameLength);

        if (ExistingIndex != -1)
        {
            GetStringEntry(ExistingIndex).bIsUnique = false;

            return { ExistingIndex, false };
        }

        // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
        return AddUnchecked(Str, Length, RawNameLength, Hash, bIsChecked);
    }

    /* returns pair<Index, bWasAdded> */
    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const std::string& String, bool bIsChecked = false)
    {
        size_t LastDotIdx = String.find_last_of('.');

        if (LastDotIdx == -1)
        {
            return FindOrAdd(String.c_str(), String.size(), bIsChecked);
        }
        else
        {
            return FindOrAdd(String.c_str(), LastDotIdx + 1, bIsChecked, String.size() - (LastDotIdx + 1));
        }
    }

public:
    inline void DebugPrintStats() const
    {
        uint64 TotalMemoryUsed = 0x0;
        uint64 TotalMemoryAllocated = 0x0;

        for (int i = 0; i < NumBuckets; i++)
        {
            TotalMemoryUsed += Buckets[i].UncheckedSize;
            TotalMemoryUsed += Buckets[i].CheckedSize;

            TotalMemoryAllocated += Buckets[i].UncheckedSizeMax;
            TotalMemoryAllocated += Buckets[i].CheckedSizeMax;

            std::cout << std::format("Bucket[{:d}] = {{ Unchecked = {:05X}, UncheckedMax = {:05X}, Checked = {:05X}, CheckedMax = {:05X} }}\n",
                i, Buckets[i].UncheckedSize, Buckets[i].UncheckedSizeMax, Buckets[i].CheckedSize, Buckets[i].CheckedSizeMax);
        }

        std::cout << std::endl;

        std::cout << std::format("TotalMemoryUsed: {:X}\n", TotalMemoryUsed);
        std::cout << std::format("TotalMemoryAllocated: {:X}\n", TotalMemoryAllocated);
        std::cout << std::format("Percentage of allocation in use: {:.3f}\n", static_cast<double>(TotalMemoryUsed) / TotalMemoryAllocated);

        std::cout << "\n" << std::endl;
    }
};