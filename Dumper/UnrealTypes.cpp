#include <format>

#include "UnrealTypes.h"
#include "NameArray.h"

#include "UnicodeNames.h"

std::string MakeNameValid(std::string&& Name)
{
	static constexpr const char* Numbers[10] =
	{
		"Zero",
		"One",
		"Two",
		"Three",
		"Four",
		"Five",
		"Six",
		"Seven",
		"Eight",
		"Nine"
	};

	if (Name == "bool")
		return "Bool";

	if (Name == "NULL")
		return "NULLL";

	/* Replace 0 with Zero or 9 with Nine, if it is the first letter of the name. */
	if (Name[0] <= '9' && Name[0] >= '0')
	{
		Name.replace(0, 1, Numbers[Name[0] - '0']);
	}
	else if (!IsUnicodeCharXIDStart(Name[0])) // Check if the character is a valid unicode start-character
	{
		/* Replace invalid starting character with 'm' character. 'm' for "member" */
		Name[0] = 'm';
	}

	for (int i = 1; i < Name.length(); i++)
	{
		if (!IsUnicodeCharXIDContinue(Name[i]))
			Name[i] = '_';
	}

	return Name;
}


FName::FName(const void* Ptr)
	: Address(static_cast<const uint8*>(Ptr))
{
}

void FName::Init(bool bForceGNames)
{
	constexpr std::array<const char*, 5> PossibleSigs = 
	{ 
		"48 8D ? ? 48 8D ? ? E8",
		"48 8D ? ? ? 48 8D ? ? E8",
		"48 8D ? ? 49 8B ? E8",
		"48 8D ? ? ? 49 8B ? E8",
		"48 8D ? ? 48 8B ? E8"
		"48 8D ? ? ? 48 8B ? E8",
	};

	MemAddress StringRef = FindByStringInAllSections("ForwardShadingQuality_");

	int i = 0;
	while (!AppendString && i < PossibleSigs.size())
	{
		AppendString = static_cast<void(*)(const void*, FString&)>(StringRef.RelativePattern(PossibleSigs[i], 0x50, -1 /* auto */));

		i++;
	}

	Off::InSDK::Name::AppendNameToString = AppendString && !bForceGNames ? GetOffset(AppendString) : 0x0;

	if (!AppendString || bForceGNames)
	{
		const bool bInitializedSuccessfully = NameArray::TryInit();

		if (bInitializedSuccessfully)
		{
			ToStr = [](const void* Name) -> std::string
			{
				if (!Settings::Internal::bUseUoutlineNumberName)
				{
					const int32 Number = FName(Name).GetNumber();

					if (Number > 0)
						return NameArray::GetNameEntry(Name).GetString() + "_" + std::to_string(Number - 1);
				}

				return NameArray::GetNameEntry(Name).GetString();
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

	/* Initialize GNames offset without committing to use GNames during the dumping process or in the SDK */
	NameArray::SetGNamesWithoutCommiting();

	std::cout << std::format("Found FName::{} at Offset 0x{:X}\n\n", (Off::InSDK::Name::bIsUsingAppendStringOverToString ? "AppendString" : "ToString"), Off::InSDK::Name::AppendNameToString);

	ToStr = [](const void* Name) -> std::string
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		return OutputString;
	};
}

void FName::Init(int32 OverrideOffset, EOffsetOverrideType OverrideType, bool bIsNamePool)
{
	if (OverrideType == EOffsetOverrideType::GNames)
	{
		const bool bInitializedSuccessfully = NameArray::TryInit(OverrideOffset, bIsNamePool);

		if (bInitializedSuccessfully)
		{
			ToStr = [](const void* Name) -> std::string
			{
				if (!Settings::Internal::bUseUoutlineNumberName)
				{
					const int32 Number = FName(Name).GetNumber();

					if (Number > 0)
						return NameArray::GetNameEntry(Name).GetString() + "_" + std::to_string(Number - 1);
				}

				return NameArray::GetNameEntry(Name).GetString();
			};
		}

		return;
	}

	AppendString = reinterpret_cast<void(*)(const void*, FString&)>(GetImageBase() + OverrideOffset);

	Off::InSDK::Name::AppendNameToString = OverrideOffset;
	Off::InSDK::Name::bIsUsingAppendStringOverToString = OverrideType == EOffsetOverrideType::AppendString;

	ToStr = [](const void* Name) -> std::string
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		return OutputString;
	};

	std::cout << std::format("Manual-Override: FName::{} --> Offset 0x{:X}\n\n", (Off::InSDK::Name::bIsUsingAppendStringOverToString ? "AppendString" : "ToString"), Off::InSDK::Name::AppendNameToString);
}

void FName::InitFallback()
{
	Off::InSDK::Name::bIsUsingAppendStringOverToString = false;

	MemAddress Conv_NameToStringAddress = FindUnrealExecFunctionByString("Conv_NameToString");

	constexpr std::array<const char*, 3> PossibleSigs =
	{
		"89 44 ? ? 48 01 ? ? E8",
		"48 89 ? ? 48 8D ? ? ? E8",
		"48 89 ? ? ? 48 89 ? ? E8",
	};

	int i = 0;
	while (!AppendString && i < PossibleSigs.size())
	{
		AppendString = static_cast<void(*)(const void*, FString&)>(Conv_NameToStringAddress.RelativePattern(PossibleSigs[i], 0x90, -1 /* auto */));

		i++;
	}

	Off::InSDK::Name::AppendNameToString = AppendString ? GetOffset(AppendString) : 0x0;
}

std::string FName::ToString() const
{
	if (!Address)
		return "None";

	std::string OutputString = ToStr(Address);

	size_t pos = OutputString.rfind('/');

	if (pos == std::string::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}

std::string FName::ToRawString() const
{
	if (!Address)
		return "None";

	return ToStr(Address);
}

std::string FName::ToValidString() const
{
	return MakeNameValid(ToString());
}

int32 FName::GetCompIdx() const 
{
	return *reinterpret_cast<const int32*>(Address + Off::FName::CompIdx);
}

int32 FName::GetNumber() const
{
	return !Settings::Internal::bUseUoutlineNumberName ? *reinterpret_cast<const int32*>(Address + Off::FName::Number) : 0x0;
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
