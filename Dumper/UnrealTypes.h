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

class FName
{
	static void(*AppendString)(void*, FString&);

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
			AppendString = reinterpret_cast<void(*)(void*, FString&)>(StringRef.RelativePattern(PossibleSigs[i], 0x60, -1 /* auto */));
			i++;
		}

		Off::InSDK::AppendNameToString = uintptr_t(AppendString) - uintptr_t(GetModuleHandle(0));

		std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";
	}

	static void Init(int32 AppendStringOffset)
	{
		AppendString = reinterpret_cast<void(*)(void*, FString&)>(uintptr_t(GetModuleHandle(0)) + AppendStringOffset);

		Off::InSDK::AppendNameToString = AppendStringOffset;

		std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";
	}

	inline std::string ToString()
	{
		static FString TempString(1024);
		
		if (!AppendString)
			Init();

		AppendString(Address, TempString);

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
