#pragma once
#include "UnrealObjects.h"

class ObjectArray
{
	static void DumpObjects();
	static void GetByIndex();
	static void GetAllPackages();



	class ObjectsIterator& begin();
	class ObjectsIterator& end();

	class ObjectsIterator
	{
		ObjectArray& IteratedArray;
		int32 CurrentIndex;

		ObjectsIterator(ObjectArray& Array);
		ObjectsIterator(ObjectArray& Array, int32 StartIndex = 0);

		ObjectsIterator& operator++();
		bool operator!=(ObjectsIterator& Other);
	};
};