#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "UnrealObjects.h"

#include "Offsets.h"

namespace fs = std::filesystem;

class ObjectArray
{
private:
	friend struct FChunkedFixedUObjectArray;
	friend struct FFixedUObjectArray;
	friend class ObjectArrayValidator;

	friend bool IsAddressValidGObjects(const uintptr_t, const struct FFixedUObjectArrayLayout&);
	friend bool IsAddressValidGObjects(const uintptr_t, const struct FChunkedFixedUObjectArrayLayout&);

private:
	static inline uint8* GObjects = nullptr;
	static inline uint32 NumElementsPerChunk = 0x10000;
	static inline uint32 SizeOfFUObjectItem = 0x18;
	static inline uint32 FUObjectItemInitialOffset = 0x0;

public:
	static inline std::string DecryptionLambdaStr;

private:
	static inline void*(*ByIndex)(void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 FUObjectItemOffset, uint32 PerChunk) = nullptr;

	static inline uint8_t* (*DecryptPtr)(void* ObjPtr) = [](void* Ptr) -> uint8* { return static_cast<uint8*>(Ptr); };

private:
	static void InitializeFUObjectItem(uint8_t* FirstItemPtr);
	static void InitializeChunkSize(uint8_t* GObjects);

public:
	static void InitDecryption(uint8_t* (*DecryptionFunction)(void* ObjPtr), const char* DecryptionLambdaAsStr);

	static void Init(bool bScanAllMemory = false, const char* const ModuleName = nullptr);


	static void Init(int32 GObjectsOffset, const FFixedUObjectArrayLayout& ObjectArrayLayout = FFixedUObjectArrayLayout(), const char* const ModuleName = nullptr);
	static void Init(int32 GObjectsOffset, int32 ElementsPerChunk, const FChunkedFixedUObjectArrayLayout& ObjectArrayLayout = FChunkedFixedUObjectArrayLayout(), const char* const ModuleName = nullptr);

	static void Init(int32 GObjectsOffset, int32 NumElementsPerChunk, bool bIsChunked);

	static void DumpObjects(const fs::path& Path, bool bWithPathname = false);

	static int32 Num();

	template<typename UEType = UEObject>
	static UEType GetByIndex(int32 Index);

	template<typename UEType = UEObject>
	static UEType FindObject(std::string FullName, EClassCastFlags RequiredType = EClassCastFlags::None);

	template<typename UEType = UEObject>
	static UEType FindObjectFast(std::string Name, EClassCastFlags RequiredType = EClassCastFlags::None);

	template<typename UEType = UEObject>
	static UEType FindObjectFastInOuter(std::string Name, std::string Outer);

	static UEClass FindClass(std::string FullName);

	static UEClass FindClassFast(std::string Name);

	class ObjectsIterator
	{
		ObjectArray& IteratedArray;
		UEObject CurrentObject;
		int32 CurrentIndex;

	public:
		ObjectsIterator(ObjectArray& Array, int32 StartIndex = 0);

		UEObject operator*();
		ObjectsIterator& operator++();
		bool operator!=(const ObjectsIterator& Other);

		int32 GetIndex() const;
	};

	ObjectsIterator begin();
	ObjectsIterator end();

	static inline void* DEBUGGetGObjects()
	{
		return GObjects;
	}
};


#define InitObjectArrayDecryption(DecryptionLambda) ObjectArray::InitDecryption(DecryptionLambda, #DecryptionLambda)