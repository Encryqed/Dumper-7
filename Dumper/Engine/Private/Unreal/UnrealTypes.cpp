
#include <format>

#include "Unreal/UnrealTypes.h"
#include "Unreal/NameArray.h"

#include "Encoding/UnicodeNames.h"

#include "Architecture.h"


std::string MakeNameValid(std::wstring&& Name)
{
	static constexpr const wchar_t* Numbers[10] =
	{
		L"Zero",
		L"One",
		L"Two",
		L"Three",
		L"Four",
		L"Five",
		L"Six",
		L"Seven",
		L"Eight",
		L"Nine"
	};

	if (Name == L"bool")
		return "Bool";

	if (Name == L"NULL")
		return "NULLL";

	/* Replace 0 with Zero or 9 with Nine, if it is the first letter of the name. */
	if (Name[0] <= '9' && Name[0] >= '0')
	{
		Name.replace(0, 1, Numbers[Name[0] - '0']);
	}
	
	std::u32string Strrr;
	Strrr += UtfN::utf_cp32_t{ 200 };

	std::u32string Utf32Name = UtfN::Utf16StringToUtf32String<std::u32string>(Name);

	bool bIsFirstIteration = true;
	for (auto It = UtfN::utf32_iterator<std::u32string::iterator>(Utf32Name); It; ++It)
	{
		if (bIsFirstIteration && !IsUnicodeCharXIDStart(Name[0]))
		{
			/* Replace invalid starting character with 'm' character. 'm' for "member" */
			Name[0] = 'm';

			bIsFirstIteration = false;
		}

		if (!IsUnicodeCharXIDContinue((*It).Get()))
			It.Replace('_');
	}

	return  UtfN::Utf32StringToUtf8String<std::string>(Utf32Name);;
}


FName::FName(const void* Ptr)
	: Address(static_cast<const uint8*>(Ptr))
{
}

void FName::Init_Windows(bool bForceGNames)
{
#ifdef PLATFORM_WINDOWS

#if defined(_WIN64)
	constexpr std::array<const char*, 6> PossibleSigs = 
	{ 
		"48 8D ? ? 48 8D ? ? E8",
		"48 8D ? ? ? 48 8D ? ? E8",
		"48 8D ? ? 49 8B ? E8",
		"48 8D ? ? ? 49 8B ? E8",
		"48 8D ? ? 48 8B ? E8",
		"48 8D ? ? ? 48 8B ? E8",
	};
#elif defined(_WIN32)
	constexpr std::array<const char*, 1> PossibleSigs = 
	{
		"8D 44 24 ? 8D 4C 24 ? 50 E8",
	};
#endif

	const void* StringRef = Platform::FindByStringInAllSections("ForwardShadingQuality_", 0x0, 0x0, Settings::General::bSearchOnlyExecutableSectionsForStrings);
	
	bool bFoundPotentiallyOverlappingSig = false;

	if (StringRef)
	{
		const char* MatchingSig = nullptr;

		for (int i = 0; !AppendString && i < PossibleSigs.size(); i++)
		{
			AppendString = static_cast<decltype(AppendString)>(Platform::FindPatternInRange(PossibleSigs[i], StringRef, 0x50, true, -1/* auto */));

			if (AppendString)
				MatchingSig = PossibleSigs[i];
		}

		// This signature partially overlaps with the signature for an inlined FName::AppendString call (see comment below)
		bFoundPotentiallyOverlappingSig = MatchingSig && strcmp(MatchingSig, "48 8D ? ? ? 48 8B ? E8") == 0;
	}

	// Test if AppendString was inlined
	if ((!AppendString || bFoundPotentiallyOverlappingSig) && !bForceGNames && StringRef)
	{
		/*
		* 0x00: 8B ? ?          mov     ecx, [...]
		* 0x03: E8 ? ? ? ?      call    FName::GetComparisonNameEntry
		* 0x08: 48 8D ? ?       lea     rdx, [...]
		* 0x0B: 48 8B C8        mov     rcx, rax
		* 0x10: E8 ? ? ? ?      call    FNameEntry::GetName
		*/
		if (void* SigScanResult = Platform::FindPatternInRange("8B ? ? E8 ? ? ? ? 48 8D ? ? ? 48 8B C8 E8 ? ? ? ?", StringRef, 0x180))
		{
			const uintptr_t ResultAsInt = reinterpret_cast<const uintptr_t>(SigScanResult);

			GetNameEntryFromName = reinterpret_cast<decltype(GetNameEntryFromName)>(Architecture_x86_64::Resolve32BitRelativeCall(ResultAsInt + 0x3));
			AppendString = reinterpret_cast<decltype(AppendString)>(Architecture_x86_64::Resolve32BitRelativeCall(ResultAsInt + 0x10));

			Off::InSDK::Name::GetNameEntryFromName = Platform::GetOffset(GetNameEntryFromName);
			Off::InSDK::Name::bIsAppendStringInlinedAndUsed = true;

			ToStr = [](const void* Name) -> std::wstring
			{
				thread_local FFreableString TempString(1024);

				AppendString(GetNameEntryFromName(FName(Name).GetCompIdx()), TempString);

				std::wstring OutputString = TempString.ToWString();
				TempString.ResetNum();

				const uint32 Number = FName(Name).GetNumber();

				if (Number > 0)
					return OutputString + L'_' + std::to_wstring(Number - 1);

				return OutputString;
			};
		}
	}

	if (AppendString == nullptr)
		AppendString = static_cast<decltype(AppendString)>(TryFindApendStringBackupStringRef_Windows());

	Off::InSDK::Name::AppendNameToString = AppendString && !bForceGNames ? Platform::GetOffset(AppendString) : 0x0;

	if (!AppendString || bForceGNames)
	{
		const bool bInitializedSuccessfully = NameArray::TryInit();

		if (bInitializedSuccessfully)
		{
			ToStr = [](const void* Name) -> std::wstring
			{
				if (!Settings::Internal::bUseOutlineNumberName)
				{
					const uint32 Number = FName(Name).GetNumber();

					if (Number > 0)
						return NameArray::GetNameEntry(Name).GetWString() + L'_' + std::to_wstring(Number - 1);
				}

				return NameArray::GetNameEntry(Name).GetWString();
			};

			return;
		}
		else /* Attempt to find FName::ToString as a final fallback */
		{
			/* Initialize GNames offset without committing to use GNames during the dumping process or in the SDK */
			NameArray::SetGNamesWithoutCommiting();
			FName::InitFallback();
		}
	}

	std::cerr << std::format("Found FName::{} at Offset 0x{:X}\n\n", (Off::InSDK::Name::bIsUsingAppendStringOverToString ? "AppendString" : "ToString"), Off::InSDK::Name::AppendNameToString);

	/* Initialize GNames offset without committing to use GNames during the dumping process or in the SDK */
	NameArray::SetGNamesWithoutCommiting();

	if (ToStr)
		return;

	ToStr = [](const void* Name) -> std::wstring
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::wstring OutputString = TempString.ToWString();
		TempString.ResetNum();

		return OutputString;
	};

#endif // PLATFORM_WINDOWS
}

void FName::Init(int32 OverrideOffset, EOffsetOverrideType OverrideType, bool bIsNamePool, const char* const ModuleName)
{
	if (OverrideType == EOffsetOverrideType::GNames)
	{
		const bool bInitializedSuccessfully = NameArray::TryInit(OverrideOffset, bIsNamePool, ModuleName);

		if (bInitializedSuccessfully)
		{
			ToStr = [](const void* Name) -> std::wstring
			{
				if (!Settings::Internal::bUseOutlineNumberName)
				{
					const uint32 Number = FName(Name).GetNumber();

					if (Number > 0)
						return NameArray::GetNameEntry(Name).GetWString() + L'_' + std::to_wstring(Number - 1);
				}

				return NameArray::GetNameEntry(Name).GetWString();
			};
		}

		return;
	}

	AppendString = reinterpret_cast<decltype(AppendString)>(Platform::GetModuleBase(ModuleName) + OverrideOffset);

	Off::InSDK::Name::AppendNameToString = OverrideOffset;
	Off::InSDK::Name::bIsUsingAppendStringOverToString = OverrideType == EOffsetOverrideType::AppendString;

	ToStr = [](const void* Name) -> std::wstring
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::wstring OutputString = TempString.ToWString();
		TempString.ResetNum();

		return OutputString;
	};

	std::cerr << std::format("Manual-Override: FName::{} --> Offset 0x{:X}\n\n", (Off::InSDK::Name::bIsUsingAppendStringOverToString ? "AppendString" : "ToString"), Off::InSDK::Name::AppendNameToString);
}


void* FName::TryFindApendStringBackupStringRef_Windows()
{

#ifdef PLATFORM_WINDOWS

#if defined(_WIN64)
	constexpr std::array<const char*, 3> PossibleSigs =
	{
		"48 8B ? 48 8B ? ? E8",
		"48 8B ? ? 48 89 ? ? E8",
		"48 8B ? 48 89 ? ? ? E8"
	};
#elif defined(_WIN32)
	constexpr std::array<const char*, 0> PossibleSigs =
	{
		// Todo I guess.
	};
#endif

	const void* StringRef = Platform::FindByStringInAllSections(L" Bone: ", 0x0, 0x0, Settings::General::bSearchOnlyExecutableSectionsForStrings);

	if (StringRef)
	{
		const char* MatchingSig = nullptr;

		// AppendString comes before the string ref, so search upwards (in IDA terms)
		const uintptr_t SigSearchStartAddress = reinterpret_cast<uintptr_t>(StringRef) - 0xB0;

		for (int i = 0; !AppendString && i < PossibleSigs.size(); i++)
		{
			AppendString = static_cast<decltype(AppendString)>(Platform::FindPatternInRange(PossibleSigs[i], SigSearchStartAddress, 0x100, true, -1/* auto */));

			if (AppendString)
				return AppendString;
		}
	}
#endif // PLATFORM_WINDOWS

	return nullptr;
}

void FName::InitFallback()
{
	Off::InSDK::Name::bIsUsingAppendStringOverToString = false;

	void* Conv_NameToStringAddress = FindUnrealExecFunctionByString("Conv_NameToString");

	constexpr std::array<const char*, 3> PossibleSigs =
	{
		"89 44 ? ? 48 01 ? ? E8",
		"48 89 ? ? 48 8D ? ? ? E8",
		"48 89 ? ? ? 48 89 ? ? E8",
	};

	int i = 0;
	while (!AppendString && i < PossibleSigs.size())
	{
		AppendString = static_cast<decltype(AppendString)>(Platform::FindPatternInRange(PossibleSigs[i], Conv_NameToStringAddress, 0x90, -1 /* auto */));

		i++;
	}

	Off::InSDK::Name::AppendNameToString = AppendString ? Platform::GetOffset(AppendString) : 0x0;
}


std::wstring FName::ToRawWString() const
{
	if (!Address)
		return L"None";

	return ToStr(Address);
}

std::wstring FName::ToWString() const
{
	std::wstring OutputString = ToRawWString();

	size_t pos = OutputString.rfind('/');

	if (pos == std::wstring::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}

std::string FName::ToRawString() const
{
	if (!Address)
		return "None";

	return UtfN::WStringToString(ToRawWString());
}

std::string FName::ToString() const
{
	if (!Address)
		return "None";

	return UtfN::WStringToString(ToWString());
}

std::string FName::ToValidString() const
{
	return MakeNameValid(ToWString());
}

int32 FName::GetCompIdx() const 
{
	return *reinterpret_cast<const int32*>(Address + Off::FName::CompIdx);
}

uint32 FName::GetNumber() const
{
	if (Settings::Internal::bUseOutlineNumberName)
		return 0x0;

	if (Settings::Internal::bUseNamePool)
		return *reinterpret_cast<const uint32*>(Address + Off::FName::Number); // The number is uint32 on versions <= UE4.23 

	return static_cast<uint32_t>(*reinterpret_cast<const int32*>(Address + Off::FName::Number));
}

bool FName::operator==(FName Other) const
{
	return GetCompIdx() == Other.GetCompIdx();
}

bool FName::operator!=(FName Other) const
{
	return GetCompIdx() != Other.GetCompIdx();
}

std::string FName::CompIdxToString(int CmpIdx)
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

void* FName::DEBUGGetAppendString()
{
	return (void*)(AppendString);
}
