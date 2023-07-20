#pragma once
#include <string>
#include <vector>
#include "UnrealObjects.h"

class ObjectArray
{
private:
	static uint8* GObjects;
	static uint32 NumElementsPerChunk;
	static uint32 SizeOfFUObjectItem;

	static inline void*(*ByIndex)(void* ObjectsArray, int32 Index, uint32 FUObjectItemSize, uint32 PerChunk) = nullptr;

public:
	static inline uint8_t* (*DecryptPtr)(void* ObjPtr) = [](void* Ptr) -> uint8_t* { return (uint8_t*)Ptr; };

public:
	static void Init();

	static void Init(int32 GObjectsOffset, int32 NumElementsPerChunk, bool bIsChunked);

	static void DumpObjects();

	static void GetAllPackages(std::unordered_map<int32_t, std::vector<int32_t>>& OutPackagesWithMembers);

	static int32 Num();

	template<typename UEType = UEObject>
	static UEType GetByIndex(int32 Index);

	template<typename UEType = UEObject>
	static UEType FindObject(std::string FullName, EClassCastFlags RequiredType = EClassCastFlags::None);

	template<typename UEType = UEObject>
	static UEType FindObjectFast(std::string Name, EClassCastFlags RequiredType = EClassCastFlags::None);

	template<typename UEType = UEObject>
	static UEType FindObjectFastInOuter(std::string Name, std::string Outer);

	template<typename UEType = UEFField>
	static UEType FindMemberInObjectFast(UEStruct Struct, std::string MemberName, EClassCastFlags TypeFlags = EClassCastFlags::None);

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
