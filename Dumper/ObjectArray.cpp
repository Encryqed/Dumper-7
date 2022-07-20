#include <iostream>
#include <fstream>
#include "ObjectArray.h"
#include "Offsets.h"
#include "Utils.h"


/* Scuffed stuff up here */
struct FChunkedFixedUObjectArray
{
	struct FUObjectItem
	{
		void* Object;
		uint8_t Pad[0x10];
	};

	FUObjectItem** Objects;
	uint8_t Pad_0[0x08];
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;

	inline bool IsValid()
	{
		if (NumChunks > 0x10 || NumChunks < 0x5)
			return false;

		if (MaxChunks > 0x25 || MaxChunks < 0x18)
			return false;

		if (NumElements > MaxElements || NumChunks > MaxChunks)
			return false;

		if (((NumElements / 0x10000) + 1) != NumChunks || MaxElements / 0x10000 != MaxChunks)
			return false;


		if (IsBadReadPtr(Objects))
			return false;

		for (int i = 0; i < NumChunks; i++)
		{
			if (IsBadReadPtr(Objects[i]))
				return false;
		}

		return true;
	}
};

struct FFixedUObjectArray
{
	struct FUObjectItem
	{
		void* Object;
		uint8_t Pad[0x10];
	};

	FUObjectItem* Objects;
	int32_t Max;
	int32_t Num;

	inline bool IsValid()
	{
		if (Num > Max)
			return false;

		if (Max < 1500000)
			return false;

		if (Num < 300000)
			return false;

		if (IsBadReadPtr(Objects))
			return false;

		if (IsBadReadPtr(Objects[5].Object))
			return false;

		if (*(int32_t*)(uintptr_t(Objects[5].Object) + 0xC) != 5)
			return false;

		return true;
	}
};


uint8* ObjectArray::GObjects = nullptr;
uint32 ObjectArray::NumElementsPerChunk = 0x10000;

/* We don't speak about this function... */
void ObjectArray::Init()
{
	std::cout << "\nDumper-7 by Encryqed & me\n\n\n";

	uintptr_t ImageBase = uintptr_t(GetModuleHandle(0));
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeader);

	uint8_t* DataSection = nullptr;
	DWORD DataSize = 0;

	for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

		if (std::string((char*)CurrentSection.Name) == ".data")
		{
			DataSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			DataSize = CurrentSection.Misc.VirtualSize;
		}
	}

	std::cout << "Searching for GObjects...\n\n";
	for (int i = 0; i < DataSize; i += 0x4)
	{
		auto FixedArray = reinterpret_cast<FFixedUObjectArray*>(DataSection + i);
		auto ChunkedArray = reinterpret_cast<FChunkedFixedUObjectArray*>(DataSection + i);

		if (FixedArray->IsValid())
		{
			GObjects = DataSection + i;
			Off::FUObjectArray::Num = 0xC;

			Off::InSDK::GObjects = uintptr_t(DataSection + i) - ImageBase;

			std::cout << "Found FFixedUObjectArray GObjects at offset 0x" << std::hex << Off::InSDK::GObjects << std::dec << "\n";

			ByIndex = [](void* ObjectsArray, int32 Index, uint32 PerChunk) -> void*
			{
				if (Index < 0 || Index > Num())
					return nullptr;

				return reinterpret_cast<FFixedUObjectArray*>(ObjectsArray)->Objects[Index].Object;
				
			};

			return;
		}
		else if (ChunkedArray->IsValid())
		{
			GObjects = DataSection + i;
			Off::FUObjectArray::Num = 0x14;

			Off::InSDK::GObjects = uintptr_t(DataSection + i) - ImageBase;

			std::cout << "Found FChunkedFixedUObjectArray GObjects at offset 0x" << std::hex << Off::InSDK::GObjects << std::dec << "\n";
			
			ByIndex = [](void* ObjectsArray, int32 Index, uint32 PerChunk) -> void*
			{
				if (Index < 0 || Index > Num())
					return nullptr;

				const int32 ChunkIndex = Index / PerChunk;
				const int32 InChunkIdx = Index % PerChunk;

				return reinterpret_cast<FChunkedFixedUObjectArray*>(ObjectsArray)->Objects[ChunkIndex][InChunkIdx].Object;
			};

			if (ObjectArray::GetByIndex(300000).GetIndex() != 300000)
			{
				NumElementsPerChunk = 0x10400;
			}

			return;
		}
	}

	std::cout << "\nGObjects couldn't be found!\n\n\n";
}

void ObjectArray::DumpObjects()
{
	std::ofstream DumpStream("GObjects-Dump.txt");

	DumpStream << "Object dump by Dumper-7\n\n";
	DumpStream << "Count: " << Num() << "\n\n\n";

	for (auto Object : ObjectArray())
	{
		DumpStream << "[" << (void*)(Object.GetIndex()) << "] " << Object.GetFullName() << "\n";
	}

	DumpStream.close();
}


std::vector<int32> ObjectArray::GetAllPackages()
{
	std::vector<int32> RetPackages(100);

	for (UEObject Object : ObjectArray())
	{
		if (Object.IsA(EClassCastFlags::UPackage))
		{
			RetPackages.push_back(Object.GetIndex());
		}
	}

	return RetPackages;
}

int32 ObjectArray::Num()
{
	return *reinterpret_cast<int32*>(GObjects + Off::FUObjectArray::Num);
}

template<typename UEType>
static UEType ObjectArray::GetByIndex(int32 Index)
{
	return UEType(ByIndex(GObjects, Index, NumElementsPerChunk));
}

template<typename UEType>
UEType ObjectArray::FindObject(std::string FullName, EClassCastFlags RequiredType)
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
UEType ObjectArray::FindObjectFast(std::string Name, EClassCastFlags RequiredType)
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

UEClass ObjectArray::FindClass(std::string FullName)
{
	return FindObject<UEClass>(FullName, EClassCastFlags::UClass);
}

UEClass ObjectArray::FindClassFast(std::string Name)
{
	return FindObjectFast<UEClass>(Name, EClassCastFlags::UClass);
}

ObjectArray::ObjectsIterator ObjectArray::begin()
{
	return ObjectsIterator(*this);
}
ObjectArray::ObjectsIterator ObjectArray::end()
{
	return ObjectsIterator(*this, Num());
}


ObjectArray::ObjectsIterator::ObjectsIterator(ObjectArray& Array, int32 StartIndex)
	: IteratedArray(Array), CurrentIndex(StartIndex), CurrentObject(ObjectArray::GetByIndex(StartIndex))
{
}

UEObject ObjectArray::ObjectsIterator::operator*()
{
	return CurrentObject;
}

ObjectArray::ObjectsIterator& ObjectArray::ObjectsIterator::operator++()
{
	CurrentObject = ObjectArray::GetByIndex(++CurrentIndex);

	while (!CurrentObject && CurrentIndex < ObjectArray::Num())
	{
		CurrentObject = ObjectArray::GetByIndex(++CurrentIndex);
	}

	return *this;
}

bool ObjectArray::ObjectsIterator::operator!=(const ObjectsIterator& Other)
{
	return CurrentIndex != Other.CurrentIndex;
}

int32 ObjectArray::ObjectsIterator::GetIndex() const
{
	return CurrentIndex;
}


/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
* 
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
void TemplateTypeCreationForObjectArray(void)
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