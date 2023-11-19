#pragma once
#include "Enums.h"

#include <cassert>
#include <format>
#include <iostream>

#define WINDOWS_IGNORE_PACKING_MISMATCH

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

#pragma pack(push, 0x1)
class StringEntry
{
private:
    friend class HashStringTable;
    friend class HashStringTableTest;

    template<typename CharType>
    friend int32 Strcmp(const CharType* String, const StringEntry& Entry);

public:
    static constexpr int32 MaxStringLength = 1 << 11;

    static constexpr int32 StringEntrySizeWithoutStr = 0x3;

private:
    // Length of object name
    uint16 Length : 11;

    // PearsonHash reduced to 5 bits --- only computed if string was added when StringTable::CurrentMode == EDuplicationCheckingMode::Check
    uint16 Hash : 5;

    // If this string uses char or wchar_t
    uint8 bIsWide : 1;

    // If this name is unique amongst others
    mutable uint8 bIsUnique : 1;

    // Allows for checking if this name is unique without adding it to the table
    mutable uint8 bIsUniqueTemp : 1;

    // Unused bits
    uint8 Unused : 5;

    union
    {
        // NOT null-terminated
        char Char[MaxStringLength];
        wchar_t WChar[MaxStringLength];
    };

private:
    inline int32 GetLengthBytes() const { return StringEntrySizeWithoutStr + Length; }

    inline int32 GetStringLength() const { return Length; }

public:
    inline bool IsUniqueInTable() const { return bIsUnique; }
    inline bool IsUnique() const { return bIsUnique && bIsUniqueTemp; }

    inline uint8 GetHash() const { return Hash; }

    inline std::string GetName() const { return std::string(Char, GetStringLength()); }
    inline std::wstring GetWideName() const { return std::wstring(WChar, GetStringLength()); }
    inline std::string_view GetNameView() const { return std::string_view(Char, GetStringLength()); }
    inline std::wstring_view GetWideNameView() const { return std::wstring_view(WChar, GetStringLength()); }
};
#pragma pack(pop, 0x1)

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
    uint32 Unused : 1;
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
        uint32 Size;
        uint32 SizeMax;
    };

private:
    StringBucket Buckets[NumBuckets];

public:
    HashStringTable(uint32 InitialBucketSize = 0x5000)
    {
        assert((InitialBucketSize) >= 0x0 && "HashStringTable(0x0, 0x0) is invalid!");

        for (int i = 0; i < NumBuckets; i++)
        {
            StringBucket& CurrentBucket = Buckets[i];
            CurrentBucket.Data = static_cast<uint8*>(malloc(InitialBucketSize));

            CurrentBucket.Size = 0x0;
            CurrentBucket.SizeMax = InitialBucketSize;
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
        }

    public:
        static inline HashBucketIterator begin(const StringBucket& Bucket) { return HashBucketIterator(Bucket, 0); }
        static inline HashBucketIterator end(const StringBucket& Bucket) { return HashBucketIterator(Bucket, Bucket.Size); }

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
    inline bool CanFit(const StringBucket& Bucket, int32 StrLengthBytes) const
    {
        const int32 EntryLength = StringEntry::StringEntrySizeWithoutStr + StrLengthBytes;

        return (Bucket.Size + EntryLength) <= Bucket.SizeMax;
    }

    inline StringEntry& GetRefToEmpty(const StringBucket& Bucket)
    {
        return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.Size);
    }

    inline const StringEntry& GetStringEntry(const StringBucket& Bucket, int32 InBucketIndex) const
    {
        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline const StringEntry& GetStringEntry(int32 BucketIndex, int32 InBucketIndex) const
    {
        assert(BucketIndex < 32 && "Bucket index out of range!");

        const StringBucket& Bucket = Buckets[BucketIndex];

        assert((InBucketIndex > 0 && InBucketIndex < Bucket.Size) && "InBucketIndex was out of range!");

        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline void ResizeBucket(StringBucket& Bucket)
    {
        int32 BucketIdx = &Bucket - Buckets;

        const uint32 OldBucketSize = Bucket.Size;
        const uint64 NewBucketSizeMax = Bucket.SizeMax * 1.5;

        uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, NewBucketSizeMax));

        assert(NewData != nullptr && "Realloc failed in function 'ResizeBucket()'.");
        
        Bucket.Data = NewData;
        Bucket.SizeMax = NewBucketSizeMax;
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> AddUnchecked(const CharType* Str, int32 Length, uint8 Hash)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = Length * sizeof(CharType);

        StringBucket& Bucket = Buckets[Hash];

        if (!CanFit(Bucket, LengthBytes))
            ResizeBucket(Bucket);

        StringEntry& NewEmptyEntry = GetRefToEmpty(Bucket);

        NewEmptyEntry.Length = Length;
        NewEmptyEntry.bIsWide = std::is_same_v<CharType, wchar_t>;
        NewEmptyEntry.Hash = Hash;

        // Initially always true, later marked as false if duplicate is found
        NewEmptyEntry.bIsUnique = true;
        NewEmptyEntry.bIsUniqueTemp = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy(NewEmptyEntry.WChar, Str, LengthBytes);

        HashStringTableIndex ReturnIndex;
        ReturnIndex.Unused = 0x0;
        ReturnIndex.HashIndex = Hash;
        ReturnIndex.InBucketOffset = Bucket.Size;

        Bucket.Size += NewEmptyEntry.GetLengthBytes();

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

        assert((Index.InBucketOffset >= 0 && Index.InBucketOffset < (Bucket.Size)) && "InBucketIndex was out of range!");

        return *reinterpret_cast<StringEntry*>(Bucket.Data + Index.InBucketOffset);
    }

    template<typename CharType>
    inline HashStringTableIndex Find(const CharType* Str, int32 Length, uint8 Hash)
    {
        constexpr bool bIsWchar = std::is_same_v<CharType, wchar_t>;

        StringBucket& Bucket = Buckets[Hash];

        /* Try to find duplications withing 'checked' regions */
        for (auto It = HashBucketIterator::begin(Bucket); It != HashBucketIterator::end(Bucket); ++It)
        {
            const StringEntry& Entry = *It;

            if (Entry.Length == Length && Entry.bIsWide == bIsWchar && Strcmp(Str, Entry) == 0)
            {
                HashStringTableIndex Idx;
                Idx.Unused = 0x0;
                Idx.HashIndex = Hash;
                Idx.InBucketOffset = It.GetInBucketIndex();

                return Idx;
            }
        }

        return HashStringTableIndex::FromInt(-1);
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const CharType* Str, int32 Length)
    {
        constexpr bool bIsWChar = std::is_same_v<CharType, wchar_t>;
        
        static_assert(!bIsWChar, "'wchar_t' is not supported by the hashing function yet!");

        if (!Str || Length <= 0 || Length > StringEntry::MaxStringLength)
        {
            std::cout << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "Length <= 0" : "Length > MaxStringLength") << std::endl;
            return { HashStringTableIndex(-1), false };
        }

        uint8 Hash = SmallPearsonHash(Str);

        HashStringTableIndex ExistingIndex = Find(Str, Length, Hash);

        if (ExistingIndex != -1)
        {
            GetStringEntry(ExistingIndex).bIsUnique = false;

            return { ExistingIndex, false };
        }

        // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
        return AddUnchecked(Str, Length, Hash);
    }

    /* returns pair<Index, bWasAdded> */
    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const std::string& String)
    {
        return FindOrAdd(String.c_str(), String.size());
    }

public:
    inline void DebugPrintStats() const
    {
        uint64 TotalMemoryUsed = 0x0;
        uint64 TotalMemoryAllocated = 0x0;

        for (int i = 0; i < NumBuckets; i++)
        {
            const StringBucket& Bucket = Buckets[i];

            TotalMemoryUsed += Bucket.Size;
            TotalMemoryAllocated += Bucket.SizeMax;

            std::cout << std::format("Bucket[{:02d}] = {{ Data = {:p}, Size = {:05X}, SizeMax = {:05X} }}\n", i, static_cast<void*>(Bucket.Data), Bucket.Size, Bucket.SizeMax);
        }

        std::cout << std::endl;

        std::cout << std::format("TotalMemoryUsed: {:X}\n", TotalMemoryUsed);
        std::cout << std::format("TotalMemoryAllocated: {:X}\n", TotalMemoryAllocated);
        std::cout << std::format("Percentage of allocation in use: {:.3f}\n", static_cast<double>(TotalMemoryUsed) / TotalMemoryAllocated);

        std::cout << "\n" << std::endl;
    }
};