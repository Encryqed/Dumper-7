#pragma once
#include <string>
#include "UnrealObjects.h"

class ObjectArray
{
private:
	static uint8* GObjects;

	static inline void* (*ByIndex)(void* ObjectsArray, int32 Index) = nullptr;

public:
	static void Initialize();

	static void DumpObjects();
	static std::vector<int32> GetAllPackages();

	static int32 Num();

	template<typename UEType = UEObject>
	static UEType GetByIndex(int32 Index);

	template<typename UEType = UEObject>
	UEType FindObject(std::string FullName, EClassCastFlags RequiredType = EClassCastFlags::None);

	template<typename UEType = UEObject>
	UEType FindObjectFast(std::string Name, EClassCastFlags RequiredType = EClassCastFlags::None);


	UEClass FindClass(std::string FullName);

	UEClass FindClassFast(std::string Name);


	class ObjectsIterator
	{
		ObjectArray& IteratedArray;
		int32 CurrentIndex;

	public:
		ObjectsIterator(ObjectArray& Array, int32 StartIndex = 0);

		UEObject operator*();
		ObjectsIterator& operator++();
		bool operator!=(ObjectsIterator& Other);
	};

	ObjectsIterator begin();
	ObjectsIterator end();
};