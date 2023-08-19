#pragma once

#include <array>
#include <string>
#include <iostream>
#include <Windows.h>
#include "Enums.h"
#include "Utils.h"
#include "Offsets.h"

extern std::string MakeNameValid(std::string&& Name);

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

		return L"";
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


class FName
{
private:
	static void(*AppendString)(void*, FString&);

	inline static std::string(*ToStr)(void* Name) = nullptr;

private:
	uint8* Address;

public:
	FName() = default;

	FName(void* Ptr);

public:
	static void Init();

	static void Init(int32 AppendStringOffset);

public:
	inline const void* GetAddress() const { return Address; }

	std::string ToString();
	std::string ToValidString();

	int32 GetCompIdx();
	int32 GetNumber();

	bool operator==(FName Other);

	bool operator!=(FName Other);

	static std::string CompIdxToString(int CmpIdx);

	static void* DEBUGGetAppendString();
};
