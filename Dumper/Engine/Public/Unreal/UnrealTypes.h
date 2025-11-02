#pragma once

#include <array>
#include <string>
#include <iostream>
#include <Windows.h>

#include "Unreal/Enums.h"
#include "OffsetFinder/Offsets.h"

#include "Encoding/UnicodeNames.h"

#include "Utils.h"
#include "UnrealContainers.h"

using namespace UC;

extern std::string MakeNameValid(std::wstring&& Name);

template<typename Type>
struct TImplementedInterface
{
	Type InterfaceClass;
	int32 PointerOffset;
	bool bImplementedByK2;
};

using FImplementedInterface = TImplementedInterface<class UEClass>;

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
	// Ughhh i know this looks ugly but i have no idea how to make this look better
#if defined(_WIN64)
	inline static void(*AppendString)(const void*, FString&) = nullptr;
#elif defined(_WIN32)
	inline static void(__thiscall* AppendString)(const void*, FString&) = nullptr;
#endif

	// Fallback when AppendString was inlined as a combination of 'FNameEntry* FName::GetNameEntry()' and 'void FNameEntry::GetPlainNameString(FString& OutStr)'.
	inline static const void* (*GetNameEntryFromName)(uint32 ComparisonIndex) = nullptr;

	inline static std::wstring(*ToStr)(const void* Name) = nullptr;

private:
	const uint8* Address;

public:
	FName() = default;

	FName(const void* Ptr);

public:
	static void Init_Windows(bool bForceGNames = false);
	static void InitFallback();

	static void Init(int32 OverrideOffset, EOffsetOverrideType OverrideType = EOffsetOverrideType::AppendString, bool bIsNamePool = false, const char* const ModuleName = Settings::General::DefaultModuleName);

private:
	static void* TryFindApendStringBackupStringRef_Windows();

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
