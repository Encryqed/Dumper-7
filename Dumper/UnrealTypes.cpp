#include "UnrealTypes.h"
#include "NameArray.h"

void(*FName::AppendString)(void*, FString&) = nullptr;


FName::FName(void* Ptr)
	: Address((uint8*)Ptr)
{
}

void FName::Init()
{
	constexpr std::array<const char*, 4> PossibleSigs = { "48 8D ? ? 48 8D ? ? E8", "48 8D ? ? ? 48 8D ? ? E8", "48 8D ? ? 49 8B ? E8", "48 8D ? ? ? 49 8B ? E8" };

	auto StringRef = FindByStringInAllSections("ForwardShadingQuality_");

	int i = 0;
	while (!AppendString && i < PossibleSigs.size())
	{
		AppendString = reinterpret_cast<void(*)(void*, FString&)>(StringRef.RelativePattern(PossibleSigs[i], 0x60, -1 /* auto */));
		i++;
	}

	Off::InSDK::AppendNameToString = AppendString ? uintptr_t(AppendString) - GetImageBase() : 0x0;

	if (!AppendString)
	{
		NameArray::Init();

		ToStr = [](void* Name) -> std::string
		{
			return NameArray::GetNameEntry(Name).GetString();
		};

		return;
	}


	std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";

	ToStr = [](void* Name) -> std::string
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		return OutputString;
	};
}

void FName::Init(int32 AppendStringOffset)
{
	AppendString = reinterpret_cast<void(*)(void*, FString&)>(GetImageBase() + AppendStringOffset);

	Off::InSDK::AppendNameToString = AppendStringOffset;

	ToStr = [](void* Name) -> std::string
	{
		thread_local FFreableString TempString(1024);

		AppendString(Name, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		return OutputString;
	};

	std::cout << "Found FName::AppendString at Offset 0x" << std::hex << Off::InSDK::AppendNameToString << "\n\n";
}

std::string FName::ToString()
{
	std::string OutputString = ToStr(Address);

	size_t pos = OutputString.rfind('/');

	if (pos == std::string::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}

int32 FName::GetCompIdx()
{
	return *reinterpret_cast<int32*>(Address + Off::FName::CompIdx);
}
int32 FName::GetNumber()
{
	return *reinterpret_cast<int32*>(Address + Off::FName::Number);
}

bool FName::operator==(FName Other)
{
	return GetCompIdx() == Other.GetCompIdx();
}

bool FName::operator!=(FName Other)
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