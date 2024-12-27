#pragma once

#include <array>
#include <string>
#include <iostream>
#include <Windows.h>
#include "Enums.h"
#include "Utils.h"
#include "Offsets.h"

#include "UnicodeNames.h"

#include "UnrealContainers.h"

using namespace UC;

extern std::string MakeNameValid(std::wstring&& Name);

/*
template<typename ValueType, typename KeyType>
class TPair
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

	inline TArray()
		: Data(nullptr), NumElements(0), MaxElements(0)
	{
	}

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

	inline int32 Num() const
	{
		return NumElements;
	}

	inline int32 Max() const
	{
		return MaxElements;
	}

	inline int32 GetSlack() const
	{
		return Max() - Num();
	}

	inline bool IsEmpty() const
	{
		return Num() <= 0;
	}

	inline bool IsValid() const
	{
		return Data != nullptr;
	}

	inline bool IsValidIndex(int32 Index) const
	{
		return Data && Index >= 0 && Index < NumElements;
	}

	inline void ResetNum()
	{
		NumElements = 0;
	}

	inline void AddIfSpaceAvailable(const T& Element)
	{
		if (GetSlack() <= 0x0)
			return;

		Data[NumElements] = Element;
		NumElements++;
	}

	inline const void* GetDataPtr() const
	{
		return Data;
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

	inline std::wstring ToWString() const
	{
		if (IsValid())
		{
			return Data;
		}

		return L"";
	}

	// The original ToString function
	inline std::string ToANSIString() const
	{
		if (IsValid())
		{
			std::wstring WData(Data);
			return std::string(WData.begin(), WData.end());
		}

		return "";
	}

	// A new ToString that converts to a utf8 string as std::string.
	inline std::string ToString() const
	{
		if (IsValid())
			return ConvertWideStrToUtf8(Data);

		return "";
	}
};
*/

class FFreableString : public FString
{
public:
	using FString::FString;

	FFreableString(uint32_t NumElementsToReserve)
	{
		if (NumElementsToReserve > 0x1000000)
			return;

		this->Data = static_cast<wchar_t*>(malloc(sizeof(wchar_t) * NumElementsToReserve));
		this->NumElements = 0;
		this->MaxElements = NumElementsToReserve;
	}

	~FFreableString()
	{
		/* If we're using FName::ToString the allocation comes from the engine and we can not free it. Just leak those 2048 bytes. */
		if (Off::InSDK::Name::bIsUsingAppendStringOverToString)
			FreeArray();
	}

public:
	inline void ResetNum()
	{
		this->NumElements = 0;
	}

private:
	inline void FreeArray()
	{
		this->NumElements = 0;
		this->MaxElements = 0;
		if (this->Data) free(this->Data);
		this->Data = nullptr;
	}
};


class FName
{
public:
	enum class EOffsetOverrideType
	{
		AppendString,
		ToString,
		GNames
	};

private:
	inline static void(*AppendString)(const void*, FString&) = nullptr;

	inline static std::wstring(*ToStr)(const void* Name) = nullptr;

private:
	const uint8* Address;

public:
	FName() = default;

	FName(const void* Ptr);

public:
	static void Init(bool bForceGNames = false);
	static void InitFallback();

	static void Init(int32 OverrideOffset, EOffsetOverrideType OverrideType = EOffsetOverrideType::AppendString, bool bIsNamePool = false, const char* const ModuleName = nullptr);

public:
	inline const void* GetAddress() const { return Address; }

	std::wstring ToWString() const;
	std::wstring ToRawWString() const;

	std::string ToString() const;
	std::string ToRawString() const;
	std::string ToValidString() const;

	int32 GetCompIdx() const;
	uint32 GetNumber() const;

	bool operator==(FName Other) const;

	bool operator!=(FName Other) const;

	static std::string CompIdxToString(int CmpIdx);

	static void* DEBUGGetAppendString();
};
