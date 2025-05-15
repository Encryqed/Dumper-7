#include "HashStringTable.h"


#pragma warning(suppress: 26495)
HashStringTable::HashStringTable(uint32 InitialBucketSize)
{
    assert((InitialBucketSize) >= 0x0 && "HashStringTable(0x0) is invalid!");

    for (int i = 0; i < NumBuckets; i++)
    {
        StringBucket& CurrentBucket = Buckets[i];
        CurrentBucket.Data = static_cast<uint8*>(malloc(InitialBucketSize));

        CurrentBucket.Size = 0x0;
        CurrentBucket.SizeMax = InitialBucketSize;
    }
}

HashStringTable::~HashStringTable()
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


bool HashStringTable::CanFit(const StringBucket& Bucket, int32 StrLengthBytes) const
{
    const int32 EntryLength = StringEntry::StringEntrySizeWithoutStr + StrLengthBytes;

    return (Bucket.Size + EntryLength) <= Bucket.SizeMax;
}

StringEntry& HashStringTable::GetRefToEmpty(const StringBucket& Bucket)
{
    return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.Size);
}

const StringEntry& HashStringTable::GetStringEntry(const StringBucket& Bucket, int32 InBucketIndex) const
{
    return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
}

const StringEntry& HashStringTable::GetStringEntry(int32 BucketIndex, int32 InBucketIndex) const
{
    assert(BucketIndex < 32 && "Bucket index out of range!");

    const StringBucket& Bucket = Buckets[BucketIndex];

    assert((InBucketIndex > 0 && InBucketIndex < Bucket.Size) && "InBucketIndex was out of range!");

    return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
}

void HashStringTable::ResizeBucket(StringBucket& Bucket)
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
std::pair<HashStringTableIndex, bool> HashStringTable::AddUnchecked(const CharType* Str, int32 Length, uint8 Hash)
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

    NewEmptyEntry.OptionalCollisionCount = 0;

    // Always copy to the WChar, memcyp only copies bytes anways
    memcpy(NewEmptyEntry.WChar, Str, LengthBytes);

    HashStringTableIndex ReturnIndex;
    ReturnIndex.Unused = 0x0;
    ReturnIndex.HashIndex = Hash;
    ReturnIndex.InBucketOffset = Bucket.Size;

    Bucket.Size += NewEmptyEntry.GetLengthBytes();

    return { ReturnIndex, true };
}

const StringEntry& HashStringTable::operator[](HashStringTableIndex Index) const
{
    return GetStringEntry(Index);
}

const HashStringTable::StringBucket& HashStringTable::GetBucket(uint32 Index) const
{
    assert(Index < NumBuckets && "Index out of range!");

    return Buckets[Index];
}

const StringEntry& HashStringTable::GetStringEntry(HashStringTableIndex Index) const
{
    assert(Index.HashIndex < 32 && "Bucket index out of range!");

    const StringBucket& Bucket = Buckets[Index.HashIndex];

    assert((Index.InBucketOffset >= 0 && Index.InBucketOffset < (Bucket.Size)) && "InBucketIndex was out of range!");

    return *reinterpret_cast<StringEntry*>(Bucket.Data + Index.InBucketOffset);
}

template<typename CharType>
HashStringTableIndex HashStringTable::Find(const CharType* Str, int32 Length, uint8 Hash)
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
inline std::pair<HashStringTableIndex, bool> HashStringTable::FindOrAdd(const CharType* Str, int32 Length, bool bShouldMarkAsDuplicated)
{
    constexpr bool bIsWChar = std::is_same_v<CharType, wchar_t>;

    static_assert(!bIsWChar, "'wchar_t' is not supported by the hashing function yet!");

    if (!Str || Length <= 0 || Length > StringEntry::MaxStringLength)
    {
        std::cerr << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "Length <= 0" : "Length > MaxStringLength") << std::endl;
        return { HashStringTableIndex(-1), false };
    }

    uint8 Hash = SmallPearsonHash(Str);

    HashStringTableIndex ExistingIndex = Find(Str, Length, Hash);

    if (ExistingIndex != -1)
    {
        const StringEntry& Entry = GetStringEntry(ExistingIndex);

        if (bShouldMarkAsDuplicated)
        {
            Entry.bIsUnique = bShouldMarkAsDuplicated;
            Entry.bIsUniqueTemp = false;
            Entry.OptionalCollisionCount++;
        }

        return { ExistingIndex, false };
    }

    // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
    return AddUnchecked(Str, Length, Hash);
}

/* returns pair<Index, bWasAdded> */
std::pair<HashStringTableIndex, bool> HashStringTable::FindOrAdd(const std::string& String, bool bShouldMarkAsDuplicated)
{
    return FindOrAdd(String.c_str(), String.size(), bShouldMarkAsDuplicated);
}

int32 HashStringTable::GetTotalUsedSize() const
{
    uint64 TotalMemoryUsed = 0x0;

    for (int i = 0; i < NumBuckets; i++)
    {
        const StringBucket& Bucket = Buckets[i];

        TotalMemoryUsed += Bucket.Size;
    }

    return TotalMemoryUsed;
}

void HashStringTable::DebugPrintStats() const
{
    uint64 TotalMemoryUsed = 0x0;
    uint64 TotalMemoryAllocated = 0x0;

    for (int i = 0; i < NumBuckets; i++)
    {
        const StringBucket& Bucket = Buckets[i];

        TotalMemoryUsed += Bucket.Size;
        TotalMemoryAllocated += Bucket.SizeMax;

        std::cerr << std::format("Bucket[{:02d}] = {{ Data = {:p}, Size = {:05X}, SizeMax = {:05X} }}\n", i, static_cast<void*>(Bucket.Data), Bucket.Size, Bucket.SizeMax);
    }

    std::cerr << std::endl;

    std::cerr << std::format("TotalMemoryUsed: {:X}\n", TotalMemoryUsed);
    std::cerr << std::format("TotalMemoryAllocated: {:X}\n", TotalMemoryAllocated);
    std::cerr << std::format("Percentage of allocation in use: {:.3f}\n", static_cast<double>(TotalMemoryUsed) / TotalMemoryAllocated);

    std::cerr << "\n" << std::endl;
}

