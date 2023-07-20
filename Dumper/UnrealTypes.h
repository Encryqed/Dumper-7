#pragma once

#include <array>
#include <string>
#include <iostream>
#include <Windows.h>
#include "Enums.h"
#include "Utils.h"
#include "Offsets.h"

template<typename ValueType, typename KeyType>
class alignas(4) TPair
{
public:
	ValueType First;
	KeyType Second;
};

template<class T>
class TArray
{
	friend class FString;

protected:
	T* Data;
	int32 NumElements;
	int32 MaxElements;

public:

	TArray() = default;

	inline TArray(int32 Num)
		:NumElements(0), MaxElements(Num), Data((T*)malloc(sizeof(T)* Num))
	{
	}

	inline T& operator[](uint32 Index)
	{
		return Data[Index];
	}
	inline const T& operator[](uint32 Index) const
	{
		return Data[Index];
	}

	inline int32 Num()
	{
		return this->NumElements;
	}

	inline int32 Max()
	{
		return this->MaxElements;
	}

	inline int32 GetSlack()
	{
		return MaxElements - NumElements;
	}

	inline bool IsValid()
	{
		return this->Data != nullptr;
	}

	inline bool IsValidIndex(int32 Index)
	{
		return Index >= 0 && Index < this->NumElements;
	}

	inline bool IsValidIndex(uint32 Index)
	{
		return Index < this->NumElements;
	}

	inline void ResetNum()
	{
		NumElements = 0;
	}

protected:
	inline void FreeArray()
	{
		NumElements = 0;
		MaxElements = 0;
		if (Data) free(Data);
		Data = nullptr;
	}
};

class FString : public TArray<wchar_t>
{
public:
	using TArray::TArray;

	inline FString(const wchar_t* WChar)
	{
		MaxElements = NumElements = *WChar ? std::wcslen(WChar) + 1 : 0;

		if (NumElements)
		{
			Data = const_cast<wchar_t*>(WChar);
		}
	}

	inline FString operator=(const wchar_t*&& Other)
	{
		return FString(Other);
	}

	inline std::wstring ToWString()
	{
		if (IsValid())
		{
			return Data;
		}
	}

	inline std::string ToString()
	{
		if (IsValid())
		{
			std::wstring WData(Data);
			return std::string(WData.begin(), WData.end());
		}
		return "";
	}
};

class FFreableString : public FString
{
public:
	using FString::FString;

	~FFreableString()
	{
		FreeArray();
	}
};

struct FNameEntryHeader
{
	uint16 bIsWide : 1;

	static constexpr uint32 ProbeHashBits = 5;
	uint16 LowercaseProbeHash : ProbeHashBits;
	uint16 Len : 10;
};

struct FNameEntry
{
	friend class FNamePool;
	friend class FNamePoolIterator;

private:
	FNameEntryHeader Header;
	union
	{
		char AnsiName[1024];
		wchar_t	WideName[1024];

		struct
		{
			int32 Id;
			uint32 Number;
		} NumberedName;
	};
};

class FNamePool
{
private:
	static constexpr uint32 FNameBlockOffsetBits = 16;
	static constexpr uint32 FNameBlockOffsets = 1 << FNameBlockOffsetBits;

public:
	static FNamePool* Pool;

private:
	void* Lock;

public:
	uint32 CurrentBlock;
	uint32 CurrentByteCursor; //Offset into the block
	uint8* Blocks[8192];

public:
	enum { Stride = 2 };
	enum { BlockSizeBytes = Stride * FNameBlockOffsets };

public:
	static void Init(int32 Offset);

	inline FNameEntry const* GetEntry(int32 Id) const
	{
		const int32 Block = Id >> FNameBlockOffsetBits;
		const int32 Offset = Id & (FNameBlockOffsets - 1);

		return reinterpret_cast<FNameEntry*>(Blocks[Block] + Stride * Offset);
	}
};

class FName
{
	static void(*AppendString)(const void*, FString&);

public:
	uint8* Address;

	FName() = default;

	FName(void* Ptr)
		: Address((uint8*)Ptr)
	{
	}

	static void Init()
	{
		std::array<const char*, 4> PossibleSigs = { "48 8D ? ? 48 8D ? ? E8", "48 8D ? ? ? 48 8D ? ? E8", "48 8D ? ? 49 8B ? E8", "48 8D ? ? ? 49 8B ? E8" };

		auto StringRef = FindByString("ForwardShadingQuality_");

		int i = 0;
		while (!AppendString && i < PossibleSigs.size())
		{
			AppendString = reinterpret_cast<void(*)(const void*, FString&)>(StringRef.RelativePattern(PossibleSigs[i], 0x60, -1 /* auto */));
			i++;
		}

		Off::InSDK::AppendNameToString = uintptr_t(AppendString) - uintptr_t(GetModuleHandle(0));

		std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";
	}

	static void Init(int32 AppendStringOffset, int32 FNamePoolOffset)
	{
		FNamePool::Init(FNamePoolOffset);

		AppendString = reinterpret_cast<void(*)(const void*, FString&)>(uintptr_t(GetModuleHandle(0)) + AppendStringOffset);

		Off::InSDK::AppendNameToString = AppendStringOffset;

		std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";
	}

	inline std::string ToString()
	{
		static thread_local FFreableString TempString(1024);
		
		if (!AppendString)
			Init();

		auto* Ptr = FNamePool::Pool->GetEntry(GetCompIdx());
		AppendString(Ptr, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		size_t pos = OutputString.rfind('/');

		if (pos == std::string::npos)
			return OutputString;

		return OutputString.substr(pos + 1);
	}

	inline int32 GetCompIdx()
	{
		return *reinterpret_cast<int32*>(Address + Off::FName::CompIdx);
	}
	inline int32 GetNumber()
	{
		return *reinterpret_cast<int32*>(Address + Off::FName::Number);
	}

	inline bool operator==(FName Other)
	{
		return GetCompIdx() == Other.GetCompIdx();
	}

	inline bool operator!=(FName Other)
	{
		return GetCompIdx() != Other.GetCompIdx();
	}

	static inline std::string CompIdxToString(int CmpIdx)
	{
		if (!Settings::Internal::bUseCasePreservingName)
		{
			struct FakeFName
			{
				int CompIdx;
				uint8 Pad[0x4];
			} Name(CmpIdx);

			return FName(&Name).ToString();
		}
		else
		{
			struct FakeFName
			{
				int CompIdx;
				uint8 Pad[0xC];
			} Name(CmpIdx);

			return FName(&Name).ToString();
		}
	}

	static inline void* DEBUGGetAppendString()
	{
		return (void*)(AppendString);
	}
};
