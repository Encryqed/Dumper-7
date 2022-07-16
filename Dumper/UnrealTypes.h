#pragma once
#include <string>
#include <Windows.h>
#include <iostream>
#include "Enums.h"
#include "Utils.h"

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

	TArray() = default;

	inline TArray(int32 Num)
		:NumElements(0), MaxElements(Num)
	{
		//Data = (T*)VirtualAlloc(nullptr, sizeof(T) * Num, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		Data = (T*)malloc(sizeof(T) * Num);
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

	inline void Free()
	{
		auto MemoryFree = reinterpret_cast<void(*)(void*)>(reinterpret_cast<uintptr_t>(GetModuleHandle(0)) + 0x15B96D0);
		MemoryFree(Data);
		Data = nullptr;
		NumElements = 0;
		MaxElements = 0;
	}
	inline void ResetNum()
	{
		NumElements = 0;
	}
};

class FString : public TArray<wchar_t>
{
public:
	inline FString() = default;

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

class FName
{
public:
	int32 ComparisonIndex;
	int32 Number;

	inline std::string ToString()
	{
		static FString TempString(1024);
		static auto AppendString = reinterpret_cast<void(*)(FName*, FString&)>(FindByString("ForwardShadingQuality_").GetCalledFunction(2));
		
		AppendString(this, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

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
