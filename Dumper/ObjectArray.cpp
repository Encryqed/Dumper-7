#include "ObjectArray.h"
#include <iostream>
#include <fstream>


/* Scuffed stuff up here */
static bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			b = true;

		return b;
	}

	return true;
};

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


void ObjectArray::Initialize()
{
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

			std::cout << "Found GObjects at offset 0x" << std::hex << uintptr_t(DataSection + i) - ImageBase<< "\n";

			ByIndex = [](void* ObjectsArray, int32 Index) -> void*
			{
				ByIndex = [](void* ObjectsArray, int32 Index) -> void*
				{
					if (Index < 0 || Index > *(int32*)(uintptr_t(ObjectsArray) + Off::FUObjectArray::Num))
						return nullptr;

					return reinterpret_cast<FFixedUObjectArray*>(ObjectsArray)->Objects[Index].Object;
				};
			};

			break;
		}
		else if (ChunkedArray->IsValid())
		{
			GObjects = DataSection + i;
			Off::FUObjectArray::Num = 0x14;

			std::cout << "Found GObjects at offset 0x" << std::hex << uintptr_t(DataSection + i) - ImageBase << "\n";
			
			ByIndex = [](void* ObjectsArray, int32 Index) -> void*
			{
				if (Index < 0 || Index > *(int32*)(uintptr_t(ObjectsArray) + Off::FUObjectArray::Num))
					return nullptr;

				const int32 ChunkIndex = Index / 0x10000;
				const int32 InChunkIdx = Index % 0x10000;

				return reinterpret_cast<FChunkedFixedUObjectArray*>(ObjectsArray)->Objects[ChunkIndex][InChunkIdx].Object;
			};

			break;
		}
	}

	std::cout << "\nFinished searching!\n\n\n";
}

void ObjectArray::DumpObjects()
{
	std::ofstream DumpStream("GObjects-Dump.txt");

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
	return UEType(ByIndex(GObjects, Index));
}

template<typename UEType>
UEType ObjectArray::FindObject(std::string FullName, EClassCastFlags RequiredType)
{
	for (UEObject Object : *this)
	{
		if (Object.IsA(RequiredType) && Object.GetFullName() == FullName)
		{
			return UEType(Object);
		}
	}

	return UEType();
}

template<typename UEType>
UEType ObjectArray::FindObjectFast(std::string Name, EClassCastFlags RequiredType)
{
	for (UEObject Object : *this)
	{
		if (Object.IsA(RequiredType) && Object.GetName() == Name)
		{
			return UEType(Object);
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
	: IteratedArray(Array), CurrentIndex(StartIndex)
{
}

UEObject ObjectArray::ObjectsIterator::operator*()
{
	return ObjectArray::GetByIndex(CurrentIndex);
}

ObjectArray::ObjectsIterator& ObjectArray::ObjectsIterator::operator++()
{
	++CurrentIndex;

	return *this;
}

bool ObjectArray::ObjectsIterator::operator!=(ObjectsIterator& Other)
{
	return CurrentIndex != Other.CurrentIndex;
}