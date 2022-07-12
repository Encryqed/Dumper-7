#pragma once
#include <string>
#include <Windows.h>
#include "Enums.h"

template<class T>
class TArray
{
	friend class FString;

protected:
	T* Data;
	int32_t NumElements;
	int32_t MaxElements;

public:
	inline T& operator[](int32_t Index)
	{
		return this->Data[Index];
	}

	inline const T& operator[](int32_t Index) const
	{
		return this->Data[Index];
	}

	inline int32_t Num()
	{
		return this->NumElements;
	}

	inline int32_t Max()
	{
		return this->MaxElements;
	}

	inline int32_t GetSlack()
	{
		return MaxElements - NumElements;
	}

	inline bool IsValid()
	{
		return this->Data != nullptr;
	}

	inline bool IsValidIndex(int32_t Index)
	{
		return Index >= 0 && Index < this->NumElements;
	}

	inline bool IsValidIndex(uint32_t Index)
	{
		return Index < this->NumElements;
	}

	inline void Free()
	{
		auto MemoryFree = reinterpret_cast<void(*)(void*)>(reinterpret_cast<uintptr_t>(GetModuleHandle(0)) + 0x15B96D0);
		MemoryFree(Data);
		Data = nullptr;
		NumElements = 0;
		MaxElements = 0;
	}
};

class FString : public TArray<wchar_t>
{
public:
	inline FString() = default;

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

class FName
{
public:
	int32_t ComparisonIndex;
	int32_t Number;

	inline std::string ToString()
	{
		FString TempString;
		auto FNameToString = reinterpret_cast<void(*)(void*, FString&)>(reinterpret_cast<uintptr_t>(GetModuleHandle(0)) + 0x168ED10);
		FNameToString(this, TempString);

		std::string OutputString = TempString.ToString();
		TempString.Free();

		size_t pos = OutputString.rfind('/');

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
