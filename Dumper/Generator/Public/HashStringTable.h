#pragma once

#include <cassert>
#include <format>
#include <iostream>

#include "Unreal/Enums.h"


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

/* Used to limit access to StringEntry::OptionalCollisionCount to authorized (friend) classes only */
struct AccessLimitedCollisionCount
{
    friend class PackageManager;
    friend class PackageInfoHandle;

private:
    uint8 CollisionCount;

public:
    AccessLimitedCollisionCount(uint8 Value)
        : CollisionCount(Value)
    {
    }
};

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

    // Optional collision count, not to be used in most cases. Implemented for use in PackageManager
    mutable uint8 OptionalCollisionCount : 5;

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

    /* To be used with PackageManager */
    AccessLimitedCollisionCount GetCollisionCount() const { return { OptionalCollisionCount }; }

    inline std::string GetName() const { return std::string(Char, GetStringLength()); }
    inline std::wstring GetWideName() const { return std::wstring(WChar, GetStringLength()); }
    inline std::string_view GetNameView() const { return std::string_view(Char, GetStringLength()); }
    inline std::wstring_view GetWideNameView() const { return std::wstring_view(WChar, GetStringLength()); }
};
#pragma pack(pop)

template<typename CharType>
inline int32 Strcmp(const CharType* String, const StringEntry& Entry)
{
    static_assert(std::is_same_v<CharType, wchar_t> || std::is_same_v<CharType, char>);

    return memcmp(Entry.Char, String, Entry.GetStringLength() * sizeof(CharType));
}

struct HashStringTableIndex
{
public:
    static constexpr int32 InvalidIndex = -1;

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

    inline operator int32() const
    {
        return *reinterpret_cast<const int32*>(this);
    }

    explicit inline operator bool() const
    {
        return *this != InvalidIndex;
    }

    inline bool operator==(HashStringTableIndex Other) const { return static_cast<uint32>(*this) == static_cast<uint32>(Other); }
    inline bool operator!=(HashStringTableIndex Other) const { return static_cast<uint32>(*this) != static_cast<uint32>(Other); }

    inline bool operator==(int32 Other) const { return static_cast<int32>(*this) == Other; }
    inline bool operator!=(int32 Other) const { return static_cast<int32>(*this) != Other; }
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
    HashStringTable(uint32 InitialBucketSize = 0x5000);
    ~HashStringTable();

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
    bool CanFit(const StringBucket& Bucket, int32 StrLengthBytes) const;

    StringEntry& GetRefToEmpty(const StringBucket& Bucket);
    const StringEntry& GetStringEntry(const StringBucket& Bucket, int32 InBucketIndex) const;
    const StringEntry& GetStringEntry(int32 BucketIndex, int32 InBucketIndex) const;

    void ResizeBucket(StringBucket& Bucket);

    template<typename CharType>
    std::pair<HashStringTableIndex, bool> AddUnchecked(const CharType* Str, int32 Length, uint8 Hash);

public:
    const StringEntry& operator[](HashStringTableIndex Index) const;

public:
    const StringBucket& GetBucket(uint32 Index) const;
    const StringEntry& GetStringEntry(HashStringTableIndex Index) const;

    template<typename CharType>
    HashStringTableIndex Find(const CharType* Str, int32 Length, uint8 Hash);

    template<typename CharType>
    std::pair<HashStringTableIndex, bool> FindOrAdd(const CharType* Str, int32 Length, bool bShouldMarkAsDuplicated = true);

    /* returns pair<Index, bWasAdded> */
    std::pair<HashStringTableIndex, bool> FindOrAdd(const std::string& String, bool bShouldMarkAsDuplicated = true);

    int32 GetTotalUsedSize() const;

public:
    void DebugPrintStats() const;
};