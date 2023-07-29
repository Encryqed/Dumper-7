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

void FNameEntry::Init()
{
	if (Settings::Internal::bUseNamePool)
	{
		// Missing

		GetStr = [](uint8* NameEntry) -> std::string
		{
			auto OffsetHeader = 0;
			const int32 Len = reinterpret_cast<uint16>(NameEntry + OffsetHeader) << ;

			// Missing
			return "Eat ass, and fuck off!";
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
			int32 NameIdx = *reinterpret_cast<int32*>(NameEntry + Off::FNameEntry::NameArray::IndexOffset);
			void* NameString = reinterpret_cast<void*>(NameEntry + Off::FNameEntry::NameArray::StringOffset);

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

	if (!NameArray)
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
				Off::NameArray::ChunkCount = i + 4;

				ByIndex = [](void* NamesArray, int32 ComparisonIndex) -> void*
				{
					const int32 ChunkIdx = ComparisonIndex / 0x4000;
					const int32 InChunk = ComparisonIndex % 0x4000;

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
	uint8_t** ChunkPtr = reinterpret_cast<uint8_t**>(NamePool + 0x10);

	// "/Script/CoreUObject"
	uint8_t* CoreUObjectFNameEntry = nullptr;

	int32 FNameEntryHeaderSize = 0x0;

	constexpr uint64 CoreUObjAsUint64 = 0x726F432F74706972; // little endian "ript/Cor" ["/Script/CoreUObject"]
	constexpr uint32 NoneAsUint32 = 0x656E6F4E; // little endian "None"

	constexpr int32 CoreUObjectStringLength = sizeof("/S");


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

	constexpr int64 NoneStrLeng = 0x4; //sizeof("None") - sizeof('\0');
	constexpr int64 BytePropertyStrLeng = 0xC; //sizeof("ByteProperty") - sizeof('\0');

	// if the first FNameEntry "None" ends at offset 6 we can be sure stride = 2. Offset 6 cannot be achieved by multipliying stride = 4 with an integer ( 6 / 4 = 1.5 [flaot)])
	int64 FNameEntryLengthShiftCount = 0x0;

	int32 HeaderOffset = FNameEntryHeaderSize == 6 ? 4 : 0; // check if Header includes ComparisonId (we don't care about it)

	uint16 BytePropertyHeader = *reinterpret_cast<uint16*>(*ChunkPtr + FNameEntryHeaderSize + NoneStrLeng + HeaderOffset);

	while (BytePropertyHeader != BytePropertyStrLeng)
	{
		FNameEntryLengthShiftCount++;
		BytePropertyHeader >>= 1;
	}

	ByIndex = [](void* NamesArray, int32 ComparisonIndex) -> void*
	{
		const int32 ChunkIdx = ComparisonIndex >> FNameBlockOffsetBits;
		const int32 InChunk = ComparisonIndex & (FNameBlockMaxOffset - 1);

		uint8_t* ChunkPtr = reinterpret_cast<uint8_t*>(NamesArray) + 0x10;

		return reinterpret_cast<uint8_t**>(ChunkPtr)[ChunkIdx] + NameEntryStride * InChunk;
	};

	FNameEntry::Init();

	std::cout << ByIndex(NamePool, *(int32_t*)((uint8_t*)ObjectArray::GetByIndex(0x0).GetAddress() + 0x18)) << std::endl;

	return 7;
}

void NameArray::Init()
{
	uintptr_t ImageBase = GetImageBase();

	std::cout << "Searching for GNames...\n\n";

	uint8_t** GNamesAddress = reinterpret_cast<uint8_t**>(FindPattern("48 89 3D ? ? ? ? 8B 87 ? ? ? ? 05 ? ? ? ? 99 81 E2 ? ? ? ?", 3, true));

	if (!GNamesAddress)
		GNamesAddress = reinterpret_cast<uint8_t**>(FindPattern("48 8D 0D ? ? ? ? E8 ? ? ? ? 4C 8B C0 C6 05", 3, true));

	if (NameArray::InitializeNameArray(*GNamesAddress))
	{
		GNames = *GNamesAddress;
		Settings::Internal::bUseNamePool = false;
		FNameEntry::Init();
		std::cout << "Found NameArray at offset: 0x" << (void*)((uint8_t*)GNamesAddress - ImageBase) << std::endl;
		return;
	}
	else if (NameArray::InitializeNamePool(reinterpret_cast<uint8_t*>(GNamesAddress))) // call may need changes
	{
		GNames = reinterpret_cast<uint8_t*>(GNamesAddress);
		Settings::Internal::bUseNamePool = true;
		std::cout << "Found NamePool at offset: 0x" << (void*)((uint8_t*)GNamesAddress - ImageBase) << std::endl;
		//FNameEntry::Init(); /* Moved to NameArray::InitializeNamePool to avoid duplicated logic */
		return;
	}

	std::cout << "\nGNames couldn't be found!\n\n\n";
}

int32 NameArray::GetNumElements()
{
	return *reinterpret_cast<int32*>(GNames + Off::NameArray::NumElements);
}
int32 NameArray::GetNumChunks()
{
	return *reinterpret_cast<int32*>(GNames + Off::NameArray::ChunkCount);
}

FNameEntry NameArray::GetNameEntry(void* Name)
{
	return ByIndex(GNames, FName(Name).GetCompIdx());
}

FNameEntry NameArray::GetNameEntry(int32 Idx)
{
	return ByIndex(GNames, Idx);
}

