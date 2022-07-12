#pragma once
#include <string>
#include <Windows.h>
#include "Enums.h"

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
	inline T& operator[](uint32 Index)
	{
		return Index < NumElements ? Data[Index] : nullptr;
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

	inline void Free()
	{
		auto MemoryFree = reinterpret_cast<void(*)(void*)>(reinterpret_cast<uintptr>(GetModuleHandle(0)) + 0x15B96D0);
		MemoryFree(Data);
		Data = nullptr;
		NumElements = 0;
		MaxElements = 0;
	}
};

class FString : public TArray<wchar>
{
public:
	inline FString() = default;

	inline FString(const wchar* WChar)
	{
		MaxElements = NumElements = *WChar ? std::wcslen(WChar) + 1 : 0;

		if (NumElements)
		{
			Data = const_cast<wchar*>(WChar);
		}
	}

	inline FString operator=(const wchar*&& Other)
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

class FName
{
public:
	int32 ComparisonIndex;
	int32 Number;

	inline std::string ToString()
	{
		FString TempString;
		auto FNameToString = reinterpret_cast<void(*)(void*, FString&)>(reinterpret_cast<uintptr>(GetModuleHandle(0)) + 0x168ED10);
		FNameToString(this, TempString);

		std::string OutputString = TempString.ToString();
		TempString.Free();

		size pos = OutputString.rfind('/');

		if (pos == std::string::npos)
			return OutputString;

		return OutputString.substr(pos + 1);
	}

	inline bool operator==(FName Other)
	{
		return ComparisonIndex == Other.ComparisonIndex;
	}

	inline bool operator!=(FName Other)
	{
		return ComparisonIndex != Other.ComparisonIndex;
	}
};
