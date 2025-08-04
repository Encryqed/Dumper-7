
#include <iostream>
#include <fstream>
#include <format>
#include <filesystem>

#include "Unreal/ObjectArray.h"
#include "OffsetFinder/Offsets.h"
#include "Utils.h"


namespace fs = std::filesystem;

constexpr inline std::array FFixedUObjectArrayLayouts =
{
	FFixedUObjectArrayLayout // Default UE4.11 - UE4.20
	{
		.ObjectsOffset = 0x0,								// 0x00
		.MaxObjectsOffset = sizeof(void*),					// 0x08 (64bit) OR 0x04 (32bit)
		.NumObjectsOffset = sizeof(void*) + sizeof(int)		// 0x0C (64bit) OR 0x08 (32bit)
	}
};

constexpr inline std::array FChunkedFixedUObjectArrayLayouts =
{
	FChunkedFixedUObjectArrayLayout // Default UE4.21 and above
	{
		.ObjectsOffset = 0x00,
		.MaxElementsOffset = 0x10,
		.NumElementsOffset = 0x14,
		.MaxChunksOffset = 0x18,
		.NumChunksOffset = 0x1C,
	},
	FChunkedFixedUObjectArrayLayout // Back4Blood
	{
		.ObjectsOffset = 0x10, // last
		.MaxElementsOffset = 0x00,
		.NumElementsOffset = 0x04,
		.MaxChunksOffset = 0x08,
		.NumChunksOffset = 0x0C,
	},
	FChunkedFixedUObjectArrayLayout // Mutliversus
	{
		.ObjectsOffset = 0x18,
		.MaxElementsOffset = 0x10,
		.NumElementsOffset = 0x00, // first
		.MaxChunksOffset = 0x14,
		.NumChunksOffset = 0x20,
	},
	FChunkedFixedUObjectArrayLayout // MindsEye
	{
		.ObjectsOffset = 0x18,
		.MaxElementsOffset = 0x00, // first
		.NumElementsOffset = 0x14,
		.MaxChunksOffset = 0x10,
		.NumChunksOffset = 0x04,
	}
};

bool IsAddressValidGObjects(const uintptr_t Address, const FFixedUObjectArrayLayout& Layout)
{
	/* It is assumed that the FUObjectItem layout is constant amongst all games using FFixedUObjectArray for ObjObjects. */
	struct FUObjectItem
	{
		void* Object;
		uint8_t Pad[sizeof(void*) * 2];
	};

	void* Objects = *reinterpret_cast<void**>(Address + Layout.ObjectsOffset);
	const int32 MaxElements = *reinterpret_cast<const int32*>(Address + Layout.MaxObjectsOffset);
	const int32 NumElements = *reinterpret_cast<const int32*>(Address + Layout.NumObjectsOffset);

	FUObjectItem* ObjectsButDecrypted = reinterpret_cast<FUObjectItem*>(ObjectArray::DecryptPtr(Objects));

	if (NumElements > MaxElements)
		return false;

	if (MaxElements > 0x400000)
		return false;

	if (NumElements < 0x1000)
		return false;

	if (IsBadReadPtr(ObjectsButDecrypted))
		return false;

	if (IsBadReadPtr(ObjectsButDecrypted[5].Object))
		return false;

	const uintptr_t FifthObject = reinterpret_cast<uintptr_t>(ObjectsButDecrypted[0x5].Object);
	const int32 IndexOfFithobject = *reinterpret_cast<int32_t*>(FifthObject + sizeof(void*) + sizeof(int32)); // FifthObject -> InternalIndex

	if (IndexOfFithobject != 0x5)
		return false;

	return true;
}

bool IsAddressValidGObjects(const uintptr_t Address, const FChunkedFixedUObjectArrayLayout& Layout)
{
	void* Objects = *reinterpret_cast<void**>(Address + Layout.ObjectsOffset);
	const int32 MaxElements = *reinterpret_cast<const int32*>(Address + Layout.MaxElementsOffset);
	const int32 NumElements = *reinterpret_cast<const int32*>(Address + Layout.NumElementsOffset);
	const int32 MaxChunks   = *reinterpret_cast<const int32*>(Address + Layout.MaxChunksOffset);
	const int32 NumChunks   = *reinterpret_cast<const int32*>(Address + Layout.NumChunksOffset);

	void** ObjectsPtrButDecrypted = reinterpret_cast<void**>(ObjectArray::DecryptPtr(Objects));

	if (NumChunks > 0x14 || NumChunks < 0x1)
		return false;

	if (MaxChunks > 0x5FF || MaxChunks < 0x6)
		return false;

	if (NumElements <= 0x800 || MaxElements <= 0x10000)
		return false;

	if (NumElements > MaxElements || NumChunks > MaxChunks)
		return false;

	if ((MaxElements % 0x10) != 0)
		return false;

	const int32_t ElementsPerChunk = MaxElements / MaxChunks;

	if ((ElementsPerChunk % 0x10) != 0)
		return false;

	if (ElementsPerChunk < 0x8000 || ElementsPerChunk > 0x80000)
		return false;

	const bool bNumChunksFitsNumElements = ((NumElements / ElementsPerChunk) + 1) == NumChunks;

	if (!bNumChunksFitsNumElements)
		return false;

	const bool bMaxChunksFitsMaxElements = (MaxElements / ElementsPerChunk) == MaxChunks;

	if (!bMaxChunksFitsMaxElements)
		return false;

	if (!ObjectsPtrButDecrypted || IsBadReadPtr(ObjectsPtrButDecrypted))
		return false;

	for (int i = 0; i < NumChunks; i++)
	{
		if (!ObjectsPtrButDecrypted[i] || IsBadReadPtr(ObjectsPtrButDecrypted[i]))
			return false;
	}

	return true;
}


void ObjectArray::InitializeFUObjectItem(uint8_t* FirstItemPtr)
{
	for (int i = 0x0; i < 0x20; i += 4)
	{
		if (!IsBadReadPtr(*reinterpret_cast<uint8_t**>(FirstItemPtr + i)))
		{
			FUObjectItemInitialOffset = i;
			break;
		}
	}

	for (int i = FUObjectItemInitialOffset + sizeof(void*); i <= 0x38; i += 4)
	{
		void* SecondObject = *reinterpret_cast<uint8**>(FirstItemPtr + i);
		void* ThirdObject  = *reinterpret_cast<uint8**>(FirstItemPtr + (i * 2) - FUObjectItemInitialOffset);

		if (!IsBadReadPtr(SecondObject) && !IsBadReadPtr(*reinterpret_cast<void**>(SecondObject)) && !IsBadReadPtr(ThirdObject) && !IsBadReadPtr(*reinterpret_cast<void**>(ThirdObject)))
		{
			SizeOfFUObjectItem = i - FUObjectItemInitialOffset;
			break;
		}
	}

	Off::InSDK::ObjArray::FUObjectItemInitialOffset = FUObjectItemInitialOffset;
	Off::InSDK::ObjArray::FUObjectItemSize = SizeOfFUObjectItem;

	std::cerr << "Off::InSDK::ObjArray::FUObjectItemSize: " << Off::InSDK::ObjArray::FUObjectItemSize << "\n" << std::endl;
}

void ObjectArray::InitDecryption(uint8_t* (*DecryptionFunction)(void* ObjPtr), const char* DecryptionLambdaAsStr)
{
	DecryptPtr = DecryptionFunction;
	DecryptionLambdaStr = DecryptionLambdaAsStr;
}


/* We don't speak about this function... */
void ObjectArray::Init(bool bScanAllMemory, const char* const ModuleName)
{
	if (!bScanAllMemory)
		std::cerr << "\nDumper-7 by me, you & him\n\n\n";

	const auto [ImageBase, ImageSize] = GetImageBaseAndSize(ModuleName);

	uintptr_t SearchBase = ImageBase;
	DWORD SearchRange = ImageSize;

	if (!bScanAllMemory)
	{
		const auto [DataSection, DataSize] = GetSectionByName(ImageBase, ".data");

		if (DataSection != 0x0 && DataSize != 0x0)
		{
			SearchBase = DataSection;
			SearchRange = DataSize;
		}
		else
		{
			bScanAllMemory = true;
		}
	}

	/* Sub 0x50 so we don't try to read out of bounds memory when checking FixedArray->IsValid() or ChunkedArray->IsValid() */
	SearchRange -= 0x50;

	if (!bScanAllMemory)
		std::cerr << "Searching for GObjects...\n\n";

	auto MatchesAnyLayout = []<typename ArrayLayoutType, size_t Size>(const std::array<ArrayLayoutType, Size>& ObjectArrayLayouts, uintptr_t Address)
	{
		for (const ArrayLayoutType& Layout : ObjectArrayLayouts)
		{
			if (!IsAddressValidGObjects(Address, Layout))
				continue;

			if constexpr (std::is_same_v<ArrayLayoutType, FFixedUObjectArrayLayout>)
			{
				Off::FUObjectArray::bIsChunked = false;
				Off::FUObjectArray::FixedLayout = Layout;
			}
			else
			{
				Off::FUObjectArray::bIsChunked = true;
				Off::FUObjectArray::ChunkedFixedLayout = Layout;
			}

			return true;
		}
		
		return false;
	};

	for (int i = 0; i < SearchRange; i += 0x4)
	{
		const uintptr_t CurrentAddress = SearchBase + i;

		if (MatchesAnyLayout(FFixedUObjectArrayLayouts, CurrentAddress))
		{
			GObjects = reinterpret_cast<uint8_t*>(SearchBase + i);
			NumElementsPerChunk = -1;

			Off::InSDK::ObjArray::GObjects = (SearchBase + i) - ImageBase;

			std::cerr << "Found FFixedUObjectArray GObjects at offset 0x" << std::hex << Off::InSDK::ObjArray::GObjects << "\n\n";

			ByIndex = [](void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 FUObjectItemOffset, uint32 PerChunk) -> void*
			{
				if (Index < 0 || Index > Num())
					return nullptr;

				uint8_t* ChunkPtr = DecryptPtr(*reinterpret_cast<uint8_t**>(ObjectsArray));

				return *reinterpret_cast<void**>(ChunkPtr + FUObjectItemOffset + (Index * FUObjectItemSize));
			};

			uint8_t* FirstItem = DecryptPtr(*reinterpret_cast<uint8_t**>(GObjects + Off::FUObjectArray::GetObjectsOffset()));

			ObjectArray::InitializeFUObjectItem(FirstItem);

			return;
		}
		else if (MatchesAnyLayout(FChunkedFixedUObjectArrayLayouts, CurrentAddress))
		{
			GObjects = reinterpret_cast<uint8_t*>(SearchBase + i);
			
			NumElementsPerChunk = Max() / MaxChunks();
			Off::InSDK::ObjArray::ChunkSize = NumElementsPerChunk;

			SizeOfFUObjectItem = sizeof(void*) + sizeof(int32) + sizeof(int32);
			FUObjectItemInitialOffset = 0x0;

			Off::InSDK::ObjArray::GObjects = (SearchBase + i) - ImageBase;

			std::cerr << "Found FChunkedFixedUObjectArray GObjects at offset 0x" << std::hex << Off::InSDK::ObjArray::GObjects << "\n\n";

			ByIndex = [](void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 FUObjectItemOffset, uint32 PerChunk) -> void*
			{
				if (Index < 0 || Index > Num())
					return nullptr;

				const int32 ChunkIndex = Index / PerChunk;
				const int32 InChunkIdx = Index % PerChunk;

				uint8_t* ChunkPtr = DecryptPtr(*reinterpret_cast<uint8_t**>(ObjectsArray));

				uint8_t* Chunk = reinterpret_cast<uint8_t**>(ChunkPtr)[ChunkIndex];
				uint8_t* ItemPtr = Chunk + (InChunkIdx * FUObjectItemSize);

				return *reinterpret_cast<void**>(ItemPtr + FUObjectItemOffset);
			};
			
			uint8_t* ChunksPtr = DecryptPtr(*reinterpret_cast<uint8_t**>(GObjects + Off::FUObjectArray::GetObjectsOffset()));

			ObjectArray::InitializeFUObjectItem(*reinterpret_cast<uint8_t**>(ChunksPtr));

			return;
		}
	}

	if (!bScanAllMemory)
	{
		ObjectArray::Init(true);
		return;
	}

	if (GObjects == nullptr)
	{
		std::cerr << "\nGObjects couldn't be found!\n\n\n";
		Sleep(3000);
		exit(1);
	}
}

void ObjectArray::Init(int32 GObjectsOffset, const FFixedUObjectArrayLayout& ObjectArrayLayout, const char* const ModuleName)
{
	GObjects = reinterpret_cast<uint8_t*>(GetModuleBase(ModuleName) + GObjectsOffset);
	Off::InSDK::ObjArray::GObjects = GObjectsOffset;

	std::cerr << "GObjects: 0x" << (void*)GObjects << "\n" << std::endl;

	Off::FUObjectArray::bIsChunked = false;
	Off::FUObjectArray::FixedLayout = ObjectArrayLayout.IsValid() ? ObjectArrayLayout : FFixedUObjectArrayLayouts[0];

	ByIndex = [](void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 FUObjectItemOffset, uint32 PerChunk) -> void*
	{
		if (Index < 0 || Index > Num())
			return nullptr;

		uint8_t* ItemPtr = *reinterpret_cast<uint8_t**>(ObjectsArray) + (Index * FUObjectItemSize);

		return *reinterpret_cast<void**>(ItemPtr + FUObjectItemOffset);
	};

	uint8_t* ChunksPtr = DecryptPtr(*reinterpret_cast<uint8_t**>(GObjects + Off::FUObjectArray::GetObjectsOffset()));

	std::cerr << "Overwrote FFixedUObjectArray GObjects to offset 0x" << std::hex << Off::InSDK::ObjArray::GObjects << "\n" << std::endl;

	ObjectArray::InitializeFUObjectItem(*reinterpret_cast<uint8_t**>(ChunksPtr));
}

void ObjectArray::Init(int32 GObjectsOffset, int32 ElementsPerChunk, const FChunkedFixedUObjectArrayLayout& ObjectArrayLayout, const char* const ModuleName)
{
	GObjects = reinterpret_cast<uint8_t*>(GetModuleBase(ModuleName) + GObjectsOffset);
	Off::InSDK::ObjArray::GObjects = GObjectsOffset;

	Off::FUObjectArray::bIsChunked = true;
	Off::FUObjectArray::ChunkedFixedLayout = ObjectArrayLayout.IsValid() ? ObjectArrayLayout : FChunkedFixedUObjectArrayLayouts[0];

	NumElementsPerChunk = ElementsPerChunk;
	Off::InSDK::ObjArray::ChunkSize = ElementsPerChunk;

	ByIndex = [](void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 FUObjectItemOffset, uint32 PerChunk) -> void*
	{
		if (Index < 0 || Index > Num())
			return nullptr;

		const int32 ChunkIndex = Index / PerChunk;
		const int32 InChunkIdx = Index % PerChunk;

		uint8_t* Chunk = (*reinterpret_cast<uint8_t***>(ObjectsArray))[ChunkIndex];
		uint8_t* ItemPtr = reinterpret_cast<uint8_t*>(Chunk) + (InChunkIdx * FUObjectItemSize);

		return *reinterpret_cast<void**>(ItemPtr + FUObjectItemOffset);
	};

	uint8_t* ChunksPtr = DecryptPtr(*reinterpret_cast<uint8_t**>(GObjects + Off::FUObjectArray::GetObjectsOffset()));

	std::cerr << "Overwrote FChunkedFixedUObjectArray GObjects to offset 0x" << std::hex << Off::InSDK::ObjArray::GObjects << "\n" << std::endl;

	ObjectArray::InitializeFUObjectItem(*reinterpret_cast<uint8_t**>(ChunksPtr));
}

void ObjectArray::DumpObjects(const fs::path& Path, bool bWithPathname)
{
	std::ofstream DumpStream(Path / "GObjects-Dump.txt");

	DumpStream << "Object dump by Dumper-7\n\n";
	DumpStream << (!Settings::Generator::GameVersion.empty() && !Settings::Generator::GameName.empty() ? (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName) + "\n\n" : "");
	DumpStream << "Count: " << Num() << "\n\n\n";

	for (auto Object : ObjectArray())
	{
		if (!bWithPathname)
		{
			DumpStream << std::format("[{:08X}] {{{}}} {}\n", Object.GetIndex(), Object.GetAddress(), Object.GetFullName());
		}
		else
		{
			DumpStream << std::format("[{:08X}] {{{}}} {}\n", Object.GetIndex(), Object.GetAddress(), Object.GetPathName());
		}
	}

	DumpStream.close();
}

void ObjectArray::DumpObjectsWithProperties(const fs::path& Path, bool bWithPathname)
{
	std::ofstream DumpStream(Path / "GObjects-Dump-WithProperties.txt");

	DumpStream << "Object dump by Dumper-7\n\n";
	DumpStream << (!Settings::Generator::GameVersion.empty() && !Settings::Generator::GameName.empty() ? (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName) + "\n\n" : "");
	DumpStream << "Count: " << Num() << "\n\n\n";

	for (auto Object : ObjectArray())
	{
		if (!bWithPathname)
		{
			DumpStream << std::format("[{:08X}] {{{}}} {}\n", Object.GetIndex(), Object.GetAddress(), Object.GetFullName());
		}
		else
		{
			DumpStream << std::format("[{:08X}] {{{}}} {}\n", Object.GetIndex(), Object.GetAddress(), Object.GetPathName());
		}

		if (Object.IsA(EClassCastFlags::Struct))
		{
			for (UEProperty Prop : Object.Cast<UEStruct>().GetProperties())
			{
				DumpStream << std::format("[{:08X}] {{{}}}     {} {}\n", Prop.GetOffset(), Prop.GetAddress(), Prop.GetPropClassName(), Prop.GetName());
			}
		}
	}

	DumpStream.close();
}


int32 ObjectArray::Num()
{
	return *reinterpret_cast<int32*>(GObjects + Off::FUObjectArray::GetNumElementsOffset());
}

int32 ObjectArray::Max()
{
	return *reinterpret_cast<int32*>(GObjects + Off::FUObjectArray::GetMaxElementsOffset());
}

int32 ObjectArray::NumChunks()
{
	return *reinterpret_cast<int32*>(GObjects + Off::FUObjectArray::GetNumChunksOffset());
}

int32 ObjectArray::MaxChunks()
{
	return *reinterpret_cast<int32*>(GObjects + Off::FUObjectArray::GetMaxChunksOffset());
}

template<typename UEType>
static UEType ObjectArray::GetByIndex(int32 Index)
{
	return UEType(ByIndex(GObjects + Off::FUObjectArray::GetObjectsOffset(), Index, SizeOfFUObjectItem, FUObjectItemInitialOffset, NumElementsPerChunk));
}

template<typename UEType>
UEType ObjectArray::FindObject(const std::string& FullName, EClassCastFlags RequiredType)
{
	for (UEObject Object : ObjectArray())
	{
		if (Object.IsA(RequiredType) && Object.GetFullName() == FullName)
		{
			return Object.Cast<UEType>();
		}
	}

	return UEType();
}

template<typename UEType>
UEType ObjectArray::FindObjectFast(const std::string& Name, EClassCastFlags RequiredType)
{
	auto ObjArray = ObjectArray();

	for (UEObject Object : ObjArray)
	{
		if (Object.IsA(RequiredType) && Object.GetName() == Name)
		{
			return Object.Cast<UEType>();
		}
	}

	return UEType();
}

template<typename UEType>
static UEType ObjectArray::FindObjectFastInOuter(const std::string& Name, std::string Outer)
{
	auto ObjArray = ObjectArray();

	for (UEObject Object : ObjArray)
	{
		if (Object.GetName() == Name && Object.GetOuter().GetName() == Outer)
		{
			return Object.Cast<UEType>();
		}
	}

	return UEType();
}

UEStruct ObjectArray::FindStruct(const std::string& Name)
{
	return FindObjectFast<UEClass>(Name, EClassCastFlags::Struct);
}

UEStruct ObjectArray::FindStructFast(const std::string& Name)
{
	return FindObjectFast<UEClass>(Name, EClassCastFlags::Struct);
}

UEClass ObjectArray::FindClass(const std::string& FullName)
{
	return FindObject<UEClass>(FullName, EClassCastFlags::Class);
}

UEClass ObjectArray::FindClassFast(const std::string& Name)
{
	return FindObjectFast<UEClass>(Name, EClassCastFlags::Class);
}

ObjectArray::ObjectsIterator ObjectArray::begin()
{
	return ObjectsIterator();
}
ObjectArray::ObjectsIterator ObjectArray::end()
{
	return ObjectsIterator(Num());
}


ObjectArray::ObjectsIterator::ObjectsIterator(int32 StartIndex)
	: CurrentIndex(StartIndex), CurrentObject(ObjectArray::GetByIndex(StartIndex))
{
}

UEObject ObjectArray::ObjectsIterator::operator*() const
{
	return CurrentObject;
}

ObjectArray::ObjectsIterator& ObjectArray::ObjectsIterator::operator++()
{
	CurrentObject = ObjectArray::GetByIndex(++CurrentIndex);

	while (!CurrentObject && CurrentIndex < (ObjectArray::Num() - 1))
	{
		CurrentObject = ObjectArray::GetByIndex(++CurrentIndex);
	}

	if (!CurrentObject && CurrentIndex == (ObjectArray::Num() - 1)) [[unlikely]]
		CurrentIndex++;

	return *this;
}

bool ObjectArray::ObjectsIterator::operator==(const ObjectsIterator& Other) const
{
	return CurrentIndex == Other.CurrentIndex;
}

bool ObjectArray::ObjectsIterator::operator!=(const ObjectsIterator& Other) const
{
	return CurrentIndex != Other.CurrentIndex;
}

int32 ObjectArray::ObjectsIterator::GetIndex() const
{
	return CurrentIndex;
}

bool AllFieldIterator::operator!=(const AllFieldIterator& Other) const
{
	return CurrentObject != Other.CurrentObject || PropertyIndex != Other.PropertyIndex;
}

AllFieldIterator& AllFieldIterator::operator++()
{
	if (CurrenStructHasMoreMembers())
	{
		PropertyIndex++;

		return *this;
	}

	IterateToNextStructWithMembers();

	return *this;
}

UEProperty AllFieldIterator::operator*() const
{
	return Fields[PropertyIndex];
}


void AllFieldIterator::IterateToNextStruct()
{
	if (IsEndIterator())
		return;

	++CurrentObject;

	while (CurrentObject != ObjectEndIterator && !IsCurrentObjectStruct())
		++CurrentObject;
}
void AllFieldIterator::IterateToNextStructWithMembers()
{
	// Loop, in case we meet a struct wihtout any properties
	while (!CurrenStructHasMoreMembers())
	{
		IterateToNextStruct();
		PropertyIndex = 0;

		if (IsEndIterator())
			return;

		Fields = GetCurrentStruct().GetProperties();
	}
}

/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
*
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
[[maybe_unused]] void TemplateTypeCreationForObjectArray(void)
{
	ObjectArray::FindObject<UEObject>("");
	ObjectArray::FindObject<UEField>("");
	ObjectArray::FindObject<UEEnum>("");
	ObjectArray::FindObject<UEStruct>("");
	ObjectArray::FindObject<UEClass>("");
	ObjectArray::FindObject<UEFunction>("");
	ObjectArray::FindObject<UEProperty>("");
	ObjectArray::FindObject<UEByteProperty>("");
	ObjectArray::FindObject<UEBoolProperty>("");
	ObjectArray::FindObject<UEObjectProperty>("");
	ObjectArray::FindObject<UEClassProperty>("");
	ObjectArray::FindObject<UEStructProperty>("");
	ObjectArray::FindObject<UEArrayProperty>("");
	ObjectArray::FindObject<UEMapProperty>("");
	ObjectArray::FindObject<UESetProperty>("");
	ObjectArray::FindObject<UEEnumProperty>("");

	ObjectArray::FindObjectFast<UEObject>("");
	ObjectArray::FindObjectFast<UEField>("");
	ObjectArray::FindObjectFast<UEEnum>("");
	ObjectArray::FindObjectFast<UEStruct>("");
	ObjectArray::FindObjectFast<UEClass>("");
	ObjectArray::FindObjectFast<UEFunction>("");
	ObjectArray::FindObjectFast<UEProperty>("");
	ObjectArray::FindObjectFast<UEByteProperty>("");
	ObjectArray::FindObjectFast<UEBoolProperty>("");
	ObjectArray::FindObjectFast<UEObjectProperty>("");
	ObjectArray::FindObjectFast<UEClassProperty>("");
	ObjectArray::FindObjectFast<UEStructProperty>("");
	ObjectArray::FindObjectFast<UEArrayProperty>("");
	ObjectArray::FindObjectFast<UEMapProperty>("");
	ObjectArray::FindObjectFast<UESetProperty>("");
	ObjectArray::FindObjectFast<UEEnumProperty>("");

	ObjectArray::FindObjectFastInOuter<UEObject>("", "");
	ObjectArray::FindObjectFastInOuter<UEField>("", "");
	ObjectArray::FindObjectFastInOuter<UEEnum>("", "");
	ObjectArray::FindObjectFastInOuter<UEStruct>("", "");
	ObjectArray::FindObjectFastInOuter<UEClass>("", "");
	ObjectArray::FindObjectFastInOuter<UEFunction>("", "");
	ObjectArray::FindObjectFastInOuter<UEProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEByteProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEBoolProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEObjectProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEClassProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEStructProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEArrayProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEMapProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UESetProperty>("", "");
	ObjectArray::FindObjectFastInOuter<UEEnumProperty>("", "");

	ObjectArray::GetByIndex<UEObject>(-1);
	ObjectArray::GetByIndex<UEField>(-1);
	ObjectArray::GetByIndex<UEEnum>(-1);
	ObjectArray::GetByIndex<UEStruct>(-1);
	ObjectArray::GetByIndex<UEClass>(-1);
	ObjectArray::GetByIndex<UEFunction>(-1);
	ObjectArray::GetByIndex<UEProperty>(-1);
	ObjectArray::GetByIndex<UEByteProperty>(-1);
	ObjectArray::GetByIndex<UEBoolProperty>(-1);
	ObjectArray::GetByIndex<UEObjectProperty>(-1);
	ObjectArray::GetByIndex<UEClassProperty>(-1);
	ObjectArray::GetByIndex<UEStructProperty>(-1);
	ObjectArray::GetByIndex<UEArrayProperty>(-1);
	ObjectArray::GetByIndex<UEMapProperty>(-1);
	ObjectArray::GetByIndex<UESetProperty>(-1);
	ObjectArray::GetByIndex<UEEnumProperty>(-1);
}
