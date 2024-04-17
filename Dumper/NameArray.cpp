#include "NameArray.h"

/* DEBUG */
#include "ObjectArray.h"

uint8* NameArray::GNames = nullptr;

FNameEntry::FNameEntry(void* Ptr)
	: Address((uint8*)Ptr)
{
}

std::string FNameEntry::GetString()
{
	if (!Address)
		return "";

	return GetStr(Address);
}

void* FNameEntry::GetAddress()
{
	return Address;
}

void FNameEntry::Init(uint8_t* FirstChunkPtr, int64 NameEntryStringOffset)
{
	if (Settings::Internal::bUseNamePool)
	{
		constexpr int64 NoneStrLen = 0x4;
		constexpr int64 BytePropertyStrLen = 0xC;

		Off::FNameEntry::NamePool::StringOffset = NameEntryStringOffset;
		Off::FNameEntry::NamePool::HeaderOffset = NameEntryStringOffset == 6 ? 4 : 0;

		uint16 BytePropertyHeader = *reinterpret_cast<uint16*>(*reinterpret_cast<uint8**>(FirstChunkPtr) + NameEntryStringOffset + NoneStrLen);

		constexpr int32 MaxAllowedShiftCount = 0xFF;

		while (BytePropertyHeader != BytePropertyStrLen && FNameEntryLengthShiftCount < MaxAllowedShiftCount)
		{
			FNameEntryLengthShiftCount++;
			BytePropertyHeader >>= 1;
		}

		GetStr = [](uint8* NameEntry) -> std::string
			{
				const uint16 HeaderWithoutNumber = *reinterpret_cast<uint16*>(NameEntry + Off::FNameEntry::NamePool::HeaderOffset);
				const int32 NameLen = HeaderWithoutNumber >> FNameEntry::FNameEntryLengthShiftCount;

				if (NameLen == 0)
				{
					const int32 EntryIdOffset = Off::FNameEntry::NamePool::StringOffset + ((Off::FNameEntry::NamePool::StringOffset == 6) * 2);

					const int32 NextEntryIndex = *reinterpret_cast<int32*>(NameEntry + EntryIdOffset);
					const int32 Number = *reinterpret_cast<int32*>(NameEntry + EntryIdOffset + sizeof(int32));

					if (Number > 0)
						return NameArray::GetNameEntry(NextEntryIndex).GetString() + "_" + std::to_string(Number - 1);

					return NameArray::GetNameEntry(NextEntryIndex).GetString();
				}

				if (HeaderWithoutNumber & NameWideMask)
				{
					std::wstring WString(reinterpret_cast<const wchar_t*>(NameEntry + Off::FNameEntry::NamePool::StringOffset), NameLen);
					return std::string(WString.begin(), WString.end());
				}

				return std::string(reinterpret_cast<const char*>(NameEntry + Off::FNameEntry::NamePool::StringOffset), NameLen);
			};
	}
	else
	{
		uint8_t* FNameEntryNone = (uint8_t*)NameArray::GetNameEntry(0x0).GetAddress();
		uint8_t* FNameEntryIdxThree = (uint8_t*)NameArray::GetNameEntry(0x3).GetAddress();
		uint8_t* FNameEntryIdxEight = (uint8_t*)NameArray::GetNameEntry(0x8).GetAddress();

		for (int i = 0; i < 0x20; i++)
		{
			if (*reinterpret_cast<uint32*>(FNameEntryNone + i) == 'enoN') // None
			{
				Off::FNameEntry::NameArray::StringOffset = i;
				break;
			}
		}

		for (int i = 0; i < 0x20; i++)
		{
			// lowest bit is bIsWide mask, shift right by 1 to get the index
			if ((*reinterpret_cast<uint32*>(FNameEntryIdxThree + i) >> 1) == 0x3 &&
				(*reinterpret_cast<uint32*>(FNameEntryIdxEight + i) >> 1) == 0x8)
			{
				Off::FNameEntry::NameArray::IndexOffset = i;
				break;
			}
		}

		GetStr = [](uint8* NameEntry) -> std::string
			{
				const int32 NameIdx = *reinterpret_cast<int32*>(NameEntry + Off::FNameEntry::NameArray::IndexOffset);
				const void* NameString = reinterpret_cast<void*>(NameEntry + Off::FNameEntry::NameArray::StringOffset);

				if (NameIdx & NameWideMask)
				{
					std::wstring WString(reinterpret_cast<const wchar_t*>(NameString));
					return std::string(WString.begin(), WString.end());
				}

				return reinterpret_cast<const char*>(NameString);
			};
	}
}

bool NameArray::InitializeNameArray(uint8_t* NameArray)
{
	int32 ValidPtrCount = 0x0;
	int32 ZeroQWordCount = 0x0;

	int32 PerChunk = 0x0;

	if (!NameArray || IsBadReadPtr(NameArray))
		return false;

	for (int i = 0; i < 0x800; i += 0x8)
	{
		uint8_t* SomePtr = *reinterpret_cast<uint8_t**>(NameArray + i);

		if (SomePtr == 0)
		{
			ZeroQWordCount++;
		}
		else if (ZeroQWordCount == 0x0 && SomePtr != nullptr)
		{
			ValidPtrCount++;
		}
		else if (ZeroQWordCount > 0 && SomePtr != 0)
		{
			int32 NumElements = *reinterpret_cast<int32_t*>(NameArray + i);
			int32 NumChunks = *reinterpret_cast<int32_t*>(NameArray + i + 4);

			if (NumChunks == ValidPtrCount)
			{
				Off::NameArray::NumElements = i;
				Off::NameArray::MaxChunkIndex = i + 4;

				ByIndex = [](void* NamesArray, int32 ComparisonIndex, int32 NamePoolBlockOffsetBits) -> void*
					{
						const int32 ChunkIdx = ComparisonIndex / 0x4000;
						const int32 InChunk = ComparisonIndex % 0x4000;

						if (ComparisonIndex > NameArray::GetNumElements())
							return nullptr;

						return reinterpret_cast<void***>(NamesArray)[ChunkIdx][InChunk];
					};

				return true;
			}
		}
	}

	return false;
}

bool NameArray::InitializeNamePool(uint8_t* NamePool)
{
	Off::NameArray::MaxChunkIndex = 0x0;
	Off::NameArray::ByteCursor = 0x4;

	Off::NameArray::ChunksStart = 0x10;

	bool bWasMaxChunkIndexFound = false;

	for (int i = 0x0; i < 0x20; i += 4)
	{
		const int32 PossibleMaxChunkIdx = *reinterpret_cast<int32*>(NamePool + i);

		if (PossibleMaxChunkIdx <= 0 || PossibleMaxChunkIdx > 0x10000)
			continue;

		int32 NotNullptrCount = 0x0;
		bool bFoundFirstPtr = false;

		for (int j = 0x0; j < 0x10000; j += 8)
		{
			const int32 ChunkOffset = i + 8 + j + (i % 8);

			if ((*reinterpret_cast<uint8_t**>(NamePool + ChunkOffset)) != nullptr)
			{
				NotNullptrCount++;

				if (!bFoundFirstPtr)
				{
					bFoundFirstPtr = true;
					Off::NameArray::ChunksStart = i + 8 + j + (i % 8);
				}
			}
		}

		if (PossibleMaxChunkIdx == (NotNullptrCount - 1))
		{
			Off::NameArray::MaxChunkIndex = i;
			Off::NameArray::ByteCursor = i + 4;
			bWasMaxChunkIndexFound = true;
			break;
		}
	}

	if (!bWasMaxChunkIndexFound)
		return false;

	constexpr uint64 CoreUObjAsUint64 = 0x6A624F5565726F43; // little endian "jbOUeroC" ["/Script/CoreUObject"]
	constexpr uint32 NoneAsUint32 = 0x656E6F4E; // little endian "None"

	constexpr int64 CoreUObjectStringLength = sizeof("/S");

	uint8_t** ChunkPtr = reinterpret_cast<uint8_t**>(NamePool + Off::NameArray::ChunksStart);

	// "/Script/CoreUObject"
	bool bFoundCoreUObjectString = false;
	int64 FNameEntryHeaderSize = 0x0;

	constexpr int32 LoopLimit = 0x1000;

	for (int i = 0; i < LoopLimit; i++)
	{
		if (*reinterpret_cast<uint32*>(*ChunkPtr + i) == NoneAsUint32 && FNameEntryHeaderSize == 0)
		{
			FNameEntryHeaderSize = i;
		}
		else if (*reinterpret_cast<uint64*>(*ChunkPtr + i) == CoreUObjAsUint64)
		{
			bFoundCoreUObjectString = true;
			break;
		}
	}

	if (!bFoundCoreUObjectString)
		return false;

	NameEntryStride = FNameEntryHeaderSize == 2 ? 2 : 4;
	Off::InSDK::NameArray::FNameEntryStride = NameEntryStride;

	ByIndex = [](void* NamesArray, int32 ComparisonIndex, int32 NamePoolBlockOffsetBits) -> void*
		{
			const int32 ChunkIdx = ComparisonIndex >> NamePoolBlockOffsetBits;
			const int32 InChunkOffset = (ComparisonIndex & ((1 << NamePoolBlockOffsetBits) - 1)) * NameEntryStride;

			const bool bIsBeyondLastChunk = ChunkIdx == NameArray::GetNumChunks() && InChunkOffset > NameArray::GetByteCursor();

			if (ChunkIdx < 0 || ChunkIdx > GetNumChunks() || bIsBeyondLastChunk)
				return nullptr;

			uint8_t* ChunkPtr = reinterpret_cast<uint8_t*>(NamesArray) + 0x10;

			return reinterpret_cast<uint8_t**>(ChunkPtr)[ChunkIdx] + InChunkOffset;
		};

	Settings::Internal::bUseNamePool = true;
	FNameEntry::Init(reinterpret_cast<uint8*>(ChunkPtr), FNameEntryHeaderSize);

	return true;
}

/*
 * Finds a call to FName::GetNames, OR a reference to GNames directly, if the call has been inlined
 *
 * returns { GetNames/GNames, bIsGNamesDirectly };
*/
inline std::pair<uintptr_t, bool> FindFNameGetNamesOrGNames(uintptr_t EnterCriticalSectionAddress, uintptr_t StartAddress)
{
	/* 2 bytes operation + 4 bytes relative offset */
	constexpr int32 ASMRelativeCallSizeBytes = 0x6;

	/* Range from "ByteProperty" which we want to search upwards for "GetNames" call */
	constexpr int32 GetNamesCallSearchRange = 0x150;

	/* Find a reference to the string "ByteProperty" in 'FName::StaticInit' */
	const uint8* BytePropertyStringAddress = static_cast<uint8*>(FindByStringInAllSections(L"ByteProperty", StartAddress));

	/* Important to prevent infinite-recursion */
	if (!BytePropertyStringAddress)
		return { 0x0, false };

	for (int i = 0; i < GetNamesCallSearchRange; i++)
	{
		/* Check upwards (yes negative indexing) for a relative call opcode */
		if (BytePropertyStringAddress[-i] != 0xFF)
			continue;

		uintptr_t CallTarget = ASMUtils::Resolve32BitSectionRelativeCall(reinterpret_cast<uintptr_t>(BytePropertyStringAddress - i));

		if (CallTarget != EnterCriticalSectionAddress)
			continue;

		uintptr_t InstructionAfterCall = reinterpret_cast<uintptr_t>(BytePropertyStringAddress - (i - ASMRelativeCallSizeBytes));

		/* Check if we're dealing with a 'call' opcode */
		if (*reinterpret_cast<const uint8*>(InstructionAfterCall) == 0xE8)
			return { ASMUtils::Resolve32BitRelativeCall(InstructionAfterCall), false };

		return { ASMUtils::Resolve32BitRelativeMove(InstructionAfterCall), true };
	}

	/* Continue and search for another reference to "ByteProperty", safe because we're checking if another string-ref was found*/
	return FindFNameGetNamesOrGNames(EnterCriticalSectionAddress, reinterpret_cast<uintptr_t>(BytePropertyStringAddress));
};

bool NameArray::TryFindNameArray()
{
	/* Type of 'static TNameEntryArray& FName::GetNames()' */
	using GetNameType = void* (*)();

	/* Range from 'FName::GetNames' which we want to search down for 'mov register, GNames' */
	constexpr int32 GetNamesCallSearchRange = 0x100;

	void* EnterCriticalSectionAddress = GetImportAddress(nullptr, "kernel32.dll", "EnterCriticalSection");

	auto [Address, bIsGNamesDirectly] = FindFNameGetNamesOrGNames(reinterpret_cast<uintptr_t>(EnterCriticalSectionAddress), GetImageBase());

	if (Address == 0x0)
		return false;

	if (bIsGNamesDirectly)
	{
		if (!IsInProcessRange(Address) || IsBadReadPtr(*reinterpret_cast<void**>(Address)))
			return false;

		Off::InSDK::NameArray::GNames = GetOffset(reinterpret_cast<void*>(Address));
		return true;
	}

	/* Call GetNames to retreive the pointer to the allocation of the name-table, used for later comparison */
	void* Names = reinterpret_cast<GetNameType>(Address)();

	for (int i = 0; i < GetNamesCallSearchRange; i++)
	{
		/* Check upwards (yes negative indexing) for a relative call opcode */
		if (*reinterpret_cast<const uint16*>(Address + i) != 0x8B48)
			continue;

		uintptr_t MoveTarget = ASMUtils::Resolve32BitRelativeMove(Address + i);

		if (!IsInProcessRange(MoveTarget))
			continue;

		void* ValueOfMoveTargetAsPtr = *reinterpret_cast<void**>(MoveTarget);

		if (IsBadReadPtr(ValueOfMoveTargetAsPtr) || ValueOfMoveTargetAsPtr != Names)
			continue;

		Off::InSDK::NameArray::GNames = GetOffset(reinterpret_cast<void*>(MoveTarget));
		return true;
	}

	return false;
}

bool NameArray::TryFindNamePool()
{
	/* Number of bytes we want to search for an indirect call to InitializeSRWLock */
	constexpr int32 InitSRWLockSearchRange = 0x50;

	/* Number of bytes we want to search for lea instruction loading the string "ByteProperty" */
	constexpr int32 BytePropertySearchRange = 0x2A0;

	/* FNamePool::FNamePool contains a call to InitializeSRWLock or RtlInitializeSRWLock, we're going to check for that later */
	//uintptr_t InitSRWLockAddress = reinterpret_cast<uintptr_t>(GetImportAddress(nullptr, "kernel32.dll", "InitializeSRWLock"));
	uintptr_t InitSRWLockAddress = reinterpret_cast<uintptr_t>(GetAddressOfImportedFunctionFromAnyModule("kernel32.dll", "InitializeSRWLock"));
	uintptr_t RtlInitSRWLockAddress = reinterpret_cast<uintptr_t>(GetAddressOfImportedFunctionFromAnyModule("ntdll.dll", "RtlInitializeSRWLock"));

	/* Singleton instance of FNamePool, which is passed as a parameter to FNamePool::FNamePool */
	void* NamePoolIntance = nullptr;

	uintptr_t SigOccurrence = 0x0;;

	uintptr_t Counter = 0x0;

	while (!NamePoolIntance)
	{
		/* add 0x1 so we don't find the same occurence again and cause an infinite loop (20min. of debugging for that) */
		if (SigOccurrence > 0x0)
			SigOccurrence += 0x1;

		/* Find the next occurence of this signature to see if that may be a call to the FNamePool constructor */
		SigOccurrence = reinterpret_cast<uintptr_t>(FindPattern("48 8D 0D ? ? ? ? E8", 0x0, true, SigOccurrence));

		if (SigOccurrence == 0x0)
			break;

		constexpr int32 SizeOfMovInstructionBytes = 0x7;

		uintptr_t PossibleConstructorAddress = ASMUtils::Resolve32BitRelativeCall(SigOccurrence + SizeOfMovInstructionBytes);

		if (!IsInProcessRange(PossibleConstructorAddress))
			continue;

		for (int i = 0; i < InitSRWLockSearchRange; i++)
		{
			/* Check for a relative call with the opcodes FF 15 00 00 00 00 */
			if (*reinterpret_cast<uint16*>(PossibleConstructorAddress + i) != 0x15FF)
				continue;

			uintptr_t RelativeCallTarget = ASMUtils::Resolve32BitSectionRelativeCall(PossibleConstructorAddress + i);

			if (!IsInProcessRange(RelativeCallTarget))
				continue;

			uintptr_t ValueOfCallTarget = *reinterpret_cast<uintptr_t*>(RelativeCallTarget);

			if (ValueOfCallTarget != InitSRWLockAddress && ValueOfCallTarget != RtlInitSRWLockAddress)
				continue;

			/* Try to find the "ByteProperty" string, as it's always referenced in FNamePool::FNamePool, so we use it to verify that we got the right function */
			void* StringRef = FindByStringInAllSections(L"ByteProperty", PossibleConstructorAddress, BytePropertySearchRange);

			/* We couldn't find a wchar_t string L"ByteProperty", now see if we can find a char string "ByteProperty" */
			if (!StringRef)
				StringRef = FindByStringInAllSections("ByteProperty", PossibleConstructorAddress, BytePropertySearchRange);

			if (StringRef)
			{
				NamePoolIntance = reinterpret_cast<void*>(ASMUtils::Resolve32BitRelativeMove(SigOccurrence));
				break;
			}
		}
	}

	if (NamePoolIntance)
	{
		Off::InSDK::NameArray::GNames = GetOffset(NamePoolIntance);
		return true;
	}

	return false;
}

bool NameArray::TryInit(bool bIsTestOnly)
{
	uintptr_t ImageBase = GetImageBase();

	uint8* GNamesAddress = nullptr;

	bool bFoundNameArray = false;
	bool bFoundnamePool = false;

	if (NameArray::TryFindNameArray())
	{
		std::cout << std::format("Found 'TNameEntryArray GNames' at offset 0x{:X}\n", Off::InSDK::NameArray::GNames) << std::endl;
		GNamesAddress = *reinterpret_cast<uint8**>(ImageBase + Off::InSDK::NameArray::GNames);// Derefernce
		Settings::Internal::bUseNamePool = false;
		bFoundNameArray = true;
	}
	else if (NameArray::TryFindNamePool())
	{
		std::cout << std::format("Found 'FNamePool GNames' at offset 0x{:X}\n", Off::InSDK::NameArray::GNames) << std::endl;
		GNamesAddress = reinterpret_cast<uint8*>(ImageBase + Off::InSDK::NameArray::GNames); // No derefernce
		Settings::Internal::bUseNamePool = true;
		bFoundnamePool = true;
	}

	if (!bFoundNameArray && !bFoundnamePool)
	{
		std::cout << "\n\nCould not find GNames!\n\n" << std::endl;
		return false;
	}

	if (bIsTestOnly)
		return false;

	if (bFoundNameArray && NameArray::InitializeNameArray(GNamesAddress))
	{
		GNames = GNamesAddress;
		Settings::Internal::bUseNamePool = false;
		FNameEntry::Init();
		return true;
	}
	else if (bFoundnamePool && NameArray::InitializeNamePool(reinterpret_cast<uint8_t*>(GNamesAddress)))
	{
		GNames = GNamesAddress;
		Settings::Internal::bUseNamePool = true;
		/* FNameEntry::Init() was moved into NameArray::InitializeNamePool to avoid duplicated logic */
		return true;
	}

	//GNames = nullptr;
	//Off::InSDK::NameArray::GNames = 0x0;
	//Settings::Internal::bUseNamePool = false;

	std::cout << "The address that was found couldn't be used by the generator, this might be due to GNames-encryption.\n" << std::endl;

	return false;
}

bool NameArray::SetGNamesWithoutCommiting()
{
	/* GNames is already set */
	if (Off::InSDK::NameArray::GNames != 0x0)
		return false;

	if (NameArray::TryFindNameArray())
	{
		std::cout << std::format("Found 'TNameEntryArray GNames' at offset 0x{:X}\n", Off::InSDK::NameArray::GNames) << std::endl;
		Settings::Internal::bUseNamePool = false;
		return true;
	}
	else if (NameArray::TryFindNamePool())
	{
		std::cout << std::format("Found 'FNamePool GNames' at offset 0x{:X}\n", Off::InSDK::NameArray::GNames) << std::endl;
		Settings::Internal::bUseNamePool = true;
		return true;
	}

	std::cout << "\n\nCould not find GNames!\n\n" << std::endl;
	return false;
}

bool NameArray::Init()
{
	uintptr_t ImageBase = GetImageBase();

	std::cout << "Searching for GNames...\n\n";

	struct Signature
	{
		const char* Pattern;
		int Relative;
	};

	std::array<Signature, 5> Signatures = { {
		{ "48 89 3D ? ? ? ? 8B 87 ? ? ? ? 05 ? ? ? ? 99 81 E2 ? ? ? ?", 3 }, // TNameEntryArray
		{ "48 8D 0D ? ? ? ? E8 ? ? ? ? 4C 8B C0 C6 05", 3 }, // FNamePool
		{ "48 8D 05 ? ? ? ? 48 83 C4 ? 5F C3 48 89 5C 24", 3 }, // FNamePool Back4Blood
		{ "48 8B 1D ? ? ? ? 48 85 DB 75 ? B9 08 04 00 00", 3 }, // FNamePool Sea Of Thieves
		{ "48 8d 0d ? ? ? ? e8 ? ? ? ? c6 05 ? ? ? ? ? 0f 10 03 ", 3 } // FNamePool Farlight 84
	} };

	uint8_t** GNamesAddress = nullptr;

	for (auto Sig : Signatures)
	{
		if (GNamesAddress = reinterpret_cast<uint8_t**>(FindPattern(Sig.Pattern, Sig.Relative, true)))
			break;
	}

	if (!GNamesAddress)
	{
		std::cout << "GNames couldn't be found\n\n" << std::endl;
		return false;
	}

	Off::InSDK::NameArray::GNames = uintptr_t(GNamesAddress) - ImageBase;

	if (NameArray::InitializeNameArray(*GNamesAddress))
	{
		GNames = *GNamesAddress;
		Settings::Internal::bUseNamePool = false;
		FNameEntry::Init();
		std::cout << "Found NameArray at offset: 0x" << std::hex << (reinterpret_cast<uintptr_t>(GNamesAddress) - ImageBase) << "\n" << std::endl;
		return true;
	}
	else if (NameArray::InitializeNamePool(reinterpret_cast<uint8_t*>(GNamesAddress)))
	{
		GNames = reinterpret_cast<uint8_t*>(GNamesAddress);
		Settings::Internal::bUseNamePool = true;
		std::cout << "Found NamePool at offset: 0x" << std::hex << (reinterpret_cast<uintptr_t>(GNamesAddress) - ImageBase) << "\n" << std::endl;
		/* FNameEntry::Init() was moved into NameArray::InitializeNamePool to avoid duplicated logic */
		return true;
	}

	GNames = nullptr;
	Off::InSDK::NameArray::GNames = 0x0;
	Settings::Internal::bUseNamePool = false;

	std::cout << "GNames couldn't be found!\n\n";

	return false;
}

void NameArray::PostInit()
{
	if (GNames && Settings::Internal::bUseNamePool)
	{
		// Reverse-order iteration because newer objects are more likely to have a chunk-index equal to NumChunks - 1

		NameArray::FNameBlockOffsetBits = 0xE;

		int i = ObjectArray::Num();
		while (i >= 0)
		{
			const int32 CurrentBlock = NameArray::GetNumChunks();

			UEObject Obj = ObjectArray::GetByIndex(i);

			if (!Obj)
			{
				i--;
				continue;
			}

			const int32 ObjNameChunkIdx = Obj.GetFName().GetCompIdx() >> NameArray::FNameBlockOffsetBits;

			if (ObjNameChunkIdx == CurrentBlock)
				break;

			if (ObjNameChunkIdx > CurrentBlock)
			{
				NameArray::FNameBlockOffsetBits++;
				i = ObjectArray::Num();
			}

			i--;
		}
		Off::InSDK::NameArray::FNamePoolBlockOffsetBits = NameArray::FNameBlockOffsetBits;

		std::cout << "\nNameArray::FNameBlockOffsetBits: 0x" << std::hex << NameArray::FNameBlockOffsetBits << "\n" << std::endl;
	}
}

int32 NameArray::GetNumChunks()
{
	return *reinterpret_cast<int32*>(GNames + Off::NameArray::MaxChunkIndex);
}

int32 NameArray::GetNumElements()
{
	return !Settings::Internal::bUseNamePool ? *reinterpret_cast<int32*>(GNames + Off::NameArray::NumElements) : 0;
}

int32 NameArray::GetByteCursor()
{
	return Settings::Internal::bUseNamePool ? *reinterpret_cast<int32*>(GNames + Off::NameArray::ByteCursor) : 0;
}

FNameEntry NameArray::GetNameEntry(void* Name)
{
	return ByIndex(GNames, FName(Name).GetCompIdx(), FNameBlockOffsetBits);
}

FNameEntry NameArray::GetNameEntry(int32 Idx)
{
	return ByIndex(GNames, Idx, FNameBlockOffsetBits);
}