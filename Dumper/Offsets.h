#pragma once
#include "Enums.h"
#include "Settings.h"

namespace Off
{
	void Init();

	//Offsets not to be used during generation but inside of the generated SDK
	namespace InSDK
	{
		void InitPE();
		void InitPE(int32 Index);

		inline int32 PEIndex;
		inline int32 PEOffset;
		inline int32 GObjects;
		inline int32 ChunkSize;
		inline int32 FUObjectItemSize;
		inline int32 AppendNameToString;
		inline uint32 FNameSize;
	}

	namespace FUObjectArray
	{
		inline uint32 Num;
	}

	namespace FField
	{
		// Fixed for CasePreserving FNames by OffsetFinder::FixupHardcodedOffsets();
		inline uint32 Vft = 0x00;
		inline uint32 Class = 0x08;
		inline uint32 Owner = 0x10;
		inline uint32 Next = 0x20;
		inline uint32 Name = 0x28;
		inline uint32 Flags = 0x30;
	}

	namespace FFieldClass
	{
		// Fixed for CasePreserving FNames by OffsetFinder::FixupHardcodedOffsets();
		inline uint32 Name = 0x00;
		inline uint32 Id = 0x08;
		inline uint32 CastFlags = 0x10;
		inline uint32 ClassFlags = 0x18;
		inline uint32 SuperClass = 0x20;
	}

	namespace FName
	{
		// These values initialized by OffsetFinder::InitUObjectOffsets()
		inline uint32 CompIdx;
		inline uint32 Number;
	}

	namespace UObject
	{
		inline uint32 Vft;
		inline uint32 Flags;
		inline uint32 Index;
		inline uint32 Class;
		inline uint32 Name;
		inline uint32 Outer;
	}

	namespace UField
	{
		inline uint32 Next;
	}
	namespace UEnum
	{
		inline uint32 Names;
	}

	namespace UStruct
	{
		inline uint32 SuperStruct;
		inline uint32 Children;
		inline uint32 ChildProperties;
		inline uint32 Size;
	}

	namespace UFunction
	{
		inline uint32 FunctionFlags;
	}

	namespace UClass
	{
		inline uint32 ClassFlags;
		inline uint32 ClassDefaultObject;
	}

	namespace UProperty
	{
		inline uint32 ElementSize;
		inline uint32 PropertyFlags;
		inline uint32 Offset_Internal;
	}

	namespace UByteProperty
	{
		inline uint32 Enum;
	}

	namespace UBoolProperty
	{
		struct UBoolPropertyBase
		{
			uint8 FieldSize;
			uint8 ByteOffset;
			uint8 ByteMask;
			uint8 FieldMask;
		};

		inline uint32 Base;
	}

	namespace UObjectProperty
	{
		inline uint32 PropertyClass;
	}

	namespace UClassProperty
	{
		inline uint32 MetaClass;
	}

	namespace UStructProperty
	{
		inline uint32 Struct;
	}

	namespace UArrayProperty
	{
		inline uint32 Inner;
	}

	namespace UMapProperty
	{
		struct UMapPropertyBase
		{
			class UProperty* KeyProperty;
			class UProperty* ValueProperty;
		};

		inline uint32 Base;
	}

	namespace USetProperty
	{
		inline uint32 ElementProp;
	}

	namespace UEnumProperty
	{
		struct UEnumPropertyBase
		{
			class UProperty* UnderlayingProperty;
			class UEnum* Enum;
		};

		inline uint32 Base;
	}
}