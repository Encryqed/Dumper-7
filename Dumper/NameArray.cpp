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

		while (BytePropertyHeader != BytePropertyStrLen)
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
			if (*reinterpret_cast<uint32*>(FNameEntryNone + i) == 0x656e6f4e /*"None" in little-endian*/)
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
			break;
		}
	}

	constexpr uint64 CoreUObjAsUint64 = 0x726F432F74706972; // little endian "ript/Cor" ["/Script/CoreUObject"]
	constexpr uint32 NoneAsUint32 = 0x656E6F4E; // little endian "None"

	constexpr int64 CoreUObjectStringLength = sizeof("/S");

	uint8_t** ChunkPtr = reinterpret_cast<uint8_t**>(NamePool + Off::NameArray::ChunksStart);

	// "/Script/CoreUObject"
	uint8_t* CoreUObjectFNameEntry = nullptr;
	int64 FNameEntryHeaderSize = 0x0;

	for (int i = 0; i < 0x1000; i++)
	{
		if (*reinterpret_cast<uint32*>(*ChunkPtr + i) == NoneAsUint32)
		{
			FNameEntryHeaderSize = i;
		}
		else if (*reinterpret_cast<uint64*>(*ChunkPtr + i) == CoreUObjAsUint64)
		{
			CoreUObjectFNameEntry = *ChunkPtr + static_cast<uint64>(i) + (CoreUObjectStringLength + FNameEntryHeaderSize);
			break;
		}
	}

	NameEntryStride = FNameEntryHeaderSize == 2 ? 2 : 4;
	Off::InSDK::FNameEntryStride = NameEntryStride;

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

void NameArray::Init()
{
	uintptr_t ImageBase = GetImageBase();

	std::cout << "Searching for GNames...\n\n";

	struct Signature
	{
		const char* Pattern;
		int Relative;
	};

	std::array<Signature, 3> Signatures = { {
		{ "48 89 3D ? ? ? ? 8B 87 ? ? ? ? 05 ? ? ? ? 99 81 E2 ? ? ? ?", 3 }, // TNameEntryArray
		{ "48 8D 0D ? ? ? ? E8 ? ? ? ? 4C 8B C0 C6 05", 3 }, // FNamePool
		{ "48 8D 05 ? ? ? ? 48 83 C4 ? 5F C3 48 89 5C 24", 3 } // FNamePool Back4Blood
	}};

	uint8_t** GNamesAddress = nullptr;

	for (auto Sig : Signatures)
	{
		if (GNamesAddress = reinterpret_cast<uint8_t**>(FindPattern(Sig.Pattern, Sig.Relative, true)))
			break;
	}

	if (!GNamesAddress)
	{
		std::cout << "\nGNames couldn't be found\n\n" << std::endl;
		exit(1);
	}

	Off::InSDK::GNames = uintptr_t(GNamesAddress) - ImageBase;

	if (NameArray::InitializeNameArray(*GNamesAddress))
	{
		GNames = *GNamesAddress;
		Settings::Internal::bUseNamePool = false;
		FNameEntry::Init();
		std::cout << "Found NameArray at offset: 0x" << std::hex << (reinterpret_cast<uintptr_t>(GNamesAddress) - ImageBase) << "\n" << std::endl;
		return;
	}
	else if (NameArray::InitializeNamePool(reinterpret_cast<uint8_t*>(GNamesAddress)))
	{
		GNames = reinterpret_cast<uint8_t*>(GNamesAddress);
		Settings::Internal::bUseNamePool = true;
		std::cout << "Found NamePool at offset: 0x" << std::hex << (reinterpret_cast<uintptr_t>(GNamesAddress) - ImageBase) << "\n" << std::endl;
		/* FNameEntry::Init() was moved into NameArray::InitializeNamePool to avoid duplicated logic */
		return;
	}

	std::cout << "\nGNames couldn't be found!\n\n\n";
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
		Off::InSDK::FNamePoolBlockOffsetBits = NameArray::FNameBlockOffsetBits;

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

