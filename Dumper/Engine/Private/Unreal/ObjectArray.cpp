
#include <iostream>
#include <fstream>
#include <format>
#include <filesystem>

#include "Unreal/ObjectArray.h"
#include "OffsetFinder/Offsets.h"
#include "Utils.h"

#include "Platform.h"


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

	if (Platform::IsBadReadPtr(ObjectsButDecrypted))
		return false;

	if (Platform::IsBadReadPtr(ObjectsButDecrypted[5].Object))
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

	if (!ObjectsPtrButDecrypted || Platform::IsBadReadPtr(ObjectsPtrButDecrypted))
		return false;

	for (int i = 0; i < NumChunks; i++)
	{
		if (!ObjectsPtrButDecrypted[i] || Platform::IsBadReadPtr(ObjectsPtrButDecrypted[i]))
			return false;
	}

	return true;
}


void ObjectArray::InitializeFUObjectItem(uint8_t* FirstItemPtr)
{
	for (int i = 0x0; i < 0x20; i += 4)
	{
		if (!Platform::IsBadReadPtr(*reinterpret_cast<uint8_t**>(FirstItemPtr + i)))
		{
			FUObjectItemInitialOffset = i;
			break;
		}
	}

	for (int i = FUObjectItemInitialOffset + sizeof(void*); i <= 0x38; i += 4)
	{
		void* SecondObject = *reinterpret_cast<uint8**>(FirstItemPtr + i);
		void* ThirdObject  = *reinterpret_cast<uint8**>(FirstItemPtr + (i * 2) - FUObjectItemInitialOffset);

		if (!Platform::IsBadReadPtr(SecondObject) && !Platform::IsBadReadPtr(*reinterpret_cast<void**>(SecondObject)) &&
			!Platform::IsBadReadPtr(ThirdObject) && !Platform::IsBadReadPtr(*reinterpret_cast<void**>(ThirdObject)))
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
	{
		std::cerr << "\nDumper-7 by me, you & him\n\n\n";
		std::cerr << "Searching for GObjects...\n\n";
	}

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

	bool bIsGObjectsChunked = false;
	auto IsAddressValidGObjects = [MatchesAnyLayout, &bIsGObjectsChunked](const void* CurrentAddress) -> bool
	{
		//std::cerr << "checking addr: " << CurrentAddress << "\n";
		if (MatchesAnyLayout(FFixedUObjectArrayLayouts, reinterpret_cast<uintptr_t>(CurrentAddress)))
		{
			bIsGObjectsChunked = false;
			return true;
		}
		else if (MatchesAnyLayout(FChunkedFixedUObjectArrayLayouts, reinterpret_cast<uintptr_t>(CurrentAddress)))
		{
			bIsGObjectsChunked = true;
			return true;
		}

		return false;
	};

	void* GObjectsAddress = nullptr;

	if (bScanAllMemory)
	{
		GObjectsAddress = Platform::IterateAllSectionsWithCallback(IsAddressValidGObjects, 0x4, 0x50, ModuleName);
	}
	else
	{
		GObjectsAddress = Platform::IterateSectionWithCallback(Platform::GetSectionInfo(".data"), IsAddressValidGObjects, 0x4, 0x50);
	}


	if (GObjectsAddress)
	{
		if (!bIsGObjectsChunked)
		{
			GObjects = static_cast<uint8*>(GObjectsAddress);
			NumElementsPerChunk = -1;

			Off::InSDK::ObjArray::GObjects = Platform::GetOffset(GObjectsAddress);

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
		}
		else
		{
			GObjects = static_cast<uint8*>(GObjectsAddress);
			
			NumElementsPerChunk = Max() / MaxChunks();
			Off::InSDK::ObjArray::ChunkSize = NumElementsPerChunk;

			SizeOfFUObjectItem = sizeof(void*) + sizeof(int32) + sizeof(int32);
			FUObjectItemInitialOffset = 0x0;

			Off::InSDK::ObjArray::GObjects = Platform::GetOffset(GObjectsAddress);

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
		}

		return;
	}

	if (!bScanAllMemory)
	{
		ObjectArray::Init(true);
		return;
	}

	if (GObjects == nullptr)
	{
		std::cerr << "\nGObjects couldn't be found, please overwrite the offset in Generator.cpp.\n\n\n";
		Sleep(10000);
		exit(1);
	}
}

void ObjectArray::Init(int32 GObjectsOffset, const FFixedUObjectArrayLayout& ObjectArrayLayout, const char* const ModuleName)
{
	GObjects = reinterpret_cast<uint8_t*>(Platform::GetModuleBase(ModuleName) + GObjectsOffset);
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
	GObjects = reinterpret_cast<uint8_t*>(Platform::GetModuleBase(ModuleName) + GObjectsOffset);
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
template UEObject ObjectArray::FindObject<UEObject>(const std::string& FullName, EClassCastFlags RequiredType);
template UEField ObjectArray::FindObject<UEField>(const std::string& FullName, EClassCastFlags RequiredType);
template UEEnum ObjectArray::FindObject<UEEnum>(const std::string& FullName, EClassCastFlags RequiredType);
template UEStruct ObjectArray::FindObject<UEStruct>(const std::string& FullName, EClassCastFlags RequiredType);
template UEClass ObjectArray::FindObject<UEClass>(const std::string& FullName, EClassCastFlags RequiredType);
template UEFunction ObjectArray::FindObject<UEFunction>(const std::string& FullName, EClassCastFlags RequiredType);
template UEProperty ObjectArray::FindObject<UEProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEByteProperty ObjectArray::FindObject<UEByteProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEBoolProperty ObjectArray::FindObject<UEBoolProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEObjectProperty ObjectArray::FindObject<UEObjectProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEClassProperty ObjectArray::FindObject<UEClassProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEStructProperty ObjectArray::FindObject<UEStructProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEArrayProperty ObjectArray::FindObject<UEArrayProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEMapProperty ObjectArray::FindObject<UEMapProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UESetProperty ObjectArray::FindObject<UESetProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEEnumProperty ObjectArray::FindObject<UEEnumProperty>(const std::string& FullName, EClassCastFlags RequiredType);

template UEObject ObjectArray::FindObjectFast<UEObject>(const std::string& FullName, EClassCastFlags RequiredType);
template UEField ObjectArray::FindObjectFast<UEField>(const std::string& FullName, EClassCastFlags RequiredType);
template UEEnum ObjectArray::FindObjectFast<UEEnum>(const std::string& FullName, EClassCastFlags RequiredType);
template UEStruct ObjectArray::FindObjectFast<UEStruct>(const std::string& FullName, EClassCastFlags RequiredType);
template UEClass ObjectArray::FindObjectFast<UEClass>(const std::string& FullName, EClassCastFlags RequiredType);
template UEFunction ObjectArray::FindObjectFast<UEFunction>(const std::string& FullName, EClassCastFlags RequiredType);
template UEProperty ObjectArray::FindObjectFast<UEProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEByteProperty ObjectArray::FindObjectFast<UEByteProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEBoolProperty ObjectArray::FindObjectFast<UEBoolProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEObjectProperty ObjectArray::FindObjectFast<UEObjectProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEClassProperty ObjectArray::FindObjectFast<UEClassProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEStructProperty ObjectArray::FindObjectFast<UEStructProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEArrayProperty ObjectArray::FindObjectFast<UEArrayProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEMapProperty ObjectArray::FindObjectFast<UEMapProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UESetProperty ObjectArray::FindObjectFast<UESetProperty>(const std::string& FullName, EClassCastFlags RequiredType);
template UEEnumProperty ObjectArray::FindObjectFast<UEEnumProperty>(const std::string& FullName, EClassCastFlags RequiredType);

template UEObject ObjectArray::FindObjectFastInOuter<UEObject>(const std::string& FullName, std::string Outer);
template UEField ObjectArray::FindObjectFastInOuter<UEField>(const std::string& FullName, std::string Outer);
template UEEnum ObjectArray::FindObjectFastInOuter<UEEnum>(const std::string& FullName, std::string Outer);
template UEStruct ObjectArray::FindObjectFastInOuter<UEStruct>(const std::string& FullName, std::string Outer);
template UEClass ObjectArray::FindObjectFastInOuter<UEClass>(const std::string& FullName, std::string Outer);
template UEFunction ObjectArray::FindObjectFastInOuter<UEFunction>(const std::string& FullName, std::string Outer);
template UEProperty ObjectArray::FindObjectFastInOuter<UEProperty>(const std::string& FullName, std::string Outer);
template UEByteProperty ObjectArray::FindObjectFastInOuter<UEByteProperty>(const std::string& FullName, std::string Outer);
template UEBoolProperty ObjectArray::FindObjectFastInOuter<UEBoolProperty>(const std::string& FullName, std::string Outer);
template UEObjectProperty ObjectArray::FindObjectFastInOuter<UEObjectProperty>(const std::string& FullName, std::string Outer);
template UEClassProperty ObjectArray::FindObjectFastInOuter<UEClassProperty>(const std::string& FullName, std::string Outer);
template UEStructProperty ObjectArray::FindObjectFastInOuter<UEStructProperty>(const std::string& FullName, std::string Outer);
template UEArrayProperty ObjectArray::FindObjectFastInOuter<UEArrayProperty>(const std::string& FullName, std::string Outer);
template UEMapProperty ObjectArray::FindObjectFastInOuter<UEMapProperty>(const std::string& FullName, std::string Outer);
template UESetProperty ObjectArray::FindObjectFastInOuter<UESetProperty>(const std::string& FullName, std::string Outer);
template UEEnumProperty ObjectArray::FindObjectFastInOuter<UEEnumProperty>(const std::string& FullName, std::string Outer);

template UEObject ObjectArray::GetByIndex<UEObject>(int32 Index);
template UEField ObjectArray::GetByIndex<UEField>(int32 Index);
template UEEnum ObjectArray::GetByIndex<UEEnum>(int32 Index);
template UEStruct ObjectArray::GetByIndex<UEStruct>(int32 Index);
template UEClass ObjectArray::GetByIndex<UEClass>(int32 Index);
template UEFunction ObjectArray::GetByIndex<UEFunction>(int32 Index);
template UEProperty ObjectArray::GetByIndex<UEProperty>(int32 Index);
template UEByteProperty ObjectArray::GetByIndex<UEByteProperty>(int32 Index);
template UEBoolProperty ObjectArray::GetByIndex<UEBoolProperty>(int32 Index);
template UEObjectProperty ObjectArray::GetByIndex<UEObjectProperty>(int32 Index);
template UEClassProperty ObjectArray::GetByIndex<UEClassProperty>(int32 Index);
template UEStructProperty ObjectArray::GetByIndex<UEStructProperty>(int32 Index);
template UEArrayProperty ObjectArray::GetByIndex<UEArrayProperty>(int32 Index);
template UEMapProperty ObjectArray::GetByIndex<UEMapProperty>(int32 Index);
template UESetProperty ObjectArray::GetByIndex<UESetProperty>(int32 Index);
template UEEnumProperty ObjectArray::GetByIndex<UEEnumProperty>(int32 Index);