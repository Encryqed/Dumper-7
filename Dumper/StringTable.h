#pragma once
#include "Enums.h"


#pragma pack(0x1)
class StringEntry
{
public:
    static constexpr int32 MaxStringLength = 512;

    static constexpr int32 StringEntrySizeWithoutStr = 0x3;

private:
    friend class StringTable;

private:
    // Length of object name
    uint16 Length : 9;

    // If this string uses char or wchar_t
    uint16 bIsWide : 1;

    // If this name is unique amongst others --- always true if RawNameLength > 0
    mutable uint16 bIsUnique : 1;

    // May be used for hashing strings in the future
    uint16 Unused : 5;

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

class StringTable
{
private:
    enum class EDuplicationCheckingMode : int32
    {
        Check,
        DontCheck
    };

private:
    uint8* Data;
    int32 UncheckedSize;
    int32 CheckedSize;
    int32 AllocationSize;
    EDuplicationCheckingMode CurrentMode;

public:
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
    inline bool AddUnchecked(const CharType* Str, int32 Length, int32 RawNameLength)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = (Length + RawNameLength) * sizeof(CharType);

        if (!CanFit(StringEntry::StringEntrySizeWithoutStr + LengthBytes))
            Resize(AllocationSize * 1.5f);

        StringEntry& NewEmpty = GetRefToEmpty();

        NewEmpty.Length = Length;
        NewEmpty.RawNameLength = RawNameLength;
        NewEmpty.bIsWide = std::is_same_v<CharType, wchar_t>;

        // Initially always true, later marked as false if duplicate is found
        NewEmpty.bIsUnique = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy((void*)(NewEmpty.WChar), Str, LengthBytes);

        const int32 LengthOfNewEntry = StringEntry::StringEntrySizeWithoutStr + LengthBytes;

        const int32 ReturnIndex = GetCurrentSize();

        (CurrentMode == EDuplicationCheckingMode::DontCheck ? UncheckedSize : CheckedSize) += LengthOfNewEntry;

        return ReturnIndex;
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

    inline const StringEntry& GetStringEntry(int32 EntryIndex) const
    {
        assert(EntryIndex < GetCurrentSize() && "Index out of bounds!");

        return *reinterpret_cast<StringEntry*>(Data + EntryIndex);
    }

    template<typename CharType>
    inline int32 Add(const CharType* Str, int32 Length, int32 RawNameLength = 0)
    {
        const int CombinedLength = Length + RawNameLength;
        const bool bIsWChar = std::is_same_v<CharType, wchar_t>;

        if (!Str || CombinedLength <= 0 || CombinedLength > StringEntry::MaxStringLength)
        {
            std::cout << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "LengthBytes <= 0" : "LengthBytes > MaxStringLength") << std::endl;
            return -1;
        }

        if (CurrentMode == EDuplicationCheckingMode::DontCheck)
            return AddUnchecked(Str, Length, RawNameLength);

        for (auto It = StringTableIterator(*this, UncheckedSize); It != this->end(); ++It)
        {
            const StringEntry& Entry = *It;

            if (Entry.bIsWide == bIsWChar && Entry.Length == Length && Strcmp(Str, Entry) == 0)
            {
                Entry.bIsUnique = false;
                return It.GetPos();
            }
        }

        // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
        return AddUnchecked(Str, Length, RawNameLength);
    }

    inline int32 Add(const std::string& String)
    {
        size_t LastDotIdx = String.find_last_of('.');

        if (LastDotIdx == -1) {
            return Add(String.c_str(), String.size() + 1, 0);
        }
        else {
            return Add(String.c_str(), LastDotIdx + 1, String.size() - LastDotIdx);
        }
    }

public: /* DEBUG */
    inline uintptr_t DEBUGGetDataPtr() const
    {
        return reinterpret_cast<uintptr_t>(Data);
    }
};