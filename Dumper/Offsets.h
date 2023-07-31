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
		inline int32 FUObjectItemInitialOffset;

		inline int32 AppendNameToString;
		inline int32 FNameSize;
	}

	namespace FUObjectArray
	{
		inline int32 Ptr;
		inline int32 Num;
	}

	namespace NameArray
	{
		inline int32 ChunkCount;
		inline int32 NumElements;
		inline int32 ByteCursor;
	}

	namespace FField
	{
		// Fixed for CasePreserving FNames by OffsetFinder::FixupHardcodedOffsets();
		inline int32 Vft = 0x00;
		inline int32 Class = 0x08;
		inline int32 Owner = 0x10;
		inline int32 Next = 0x20;
		inline int32 Name = 0x28;
		inline int32 Flags = 0x30;
	}

	namespace FFieldClass
	{
		// Fixed for CasePreserving FNames by OffsetFinder::FixupHardcodedOffsets();
		inline int32 Name = 0x00;
		inline int32 Id = 0x08;
		inline int32 CastFlags = 0x10;
		inline int32 ClassFlags = 0x18;
		inline int32 SuperClass = 0x20;
	}

	namespace FName
	{
		// These values initialized by OffsetFinder::InitUObjectOffsets()
		inline int32 CompIdx;
		inline int32 Number;
	}

	namespace FNameEntry
	{
		// These values are initialized by FNameEntry::Init()
		namespace NameArray
		{
			inline int32 StringOffset;
			inline int32 IndexOffset;
		}

		// These values are initialized by FNameEntry::Init()
		namespace NamePool
		{
			inline int32 HeaderOffset;
			inline int32 StringOffset;
		}
	}

	namespace UObject
	{
		inline int32 Vft;
		inline int32 Flags;
		inline int32 Index;
		inline int32 Class;
		inline int32 Name;
		inline int32 Outer;
	}

	namespace UField
	{
		inline int32 Next;
	}
	namespace UEnum
	{
		inline int32 Names;
	}

	namespace UStruct
	{
		inline int32 SuperStruct;
		inline int32 Children;
		inline int32 ChildProperties;
		inline int32 Size;
	}

	namespace UFunction
	{
		inline int32 FunctionFlags;
	}

	namespace UClass
	{
		inline int32 CastFlags;
		inline int32 ClassDefaultObject;
	}

	namespace UProperty
	{
		inline int32 ArrayDim;
		inline int32 ElementSize;
		inline int32 PropertyFlags;
		inline int32 Offset_Internal;
	}

	namespace UByteProperty
	{
		inline int32 Enum;
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

		inline int32 Base;
	}

	namespace UObjectProperty
	{
		inline int32 PropertyClass;
	}

	namespace UClassProperty
	{
		inline int32 MetaClass;
	}

	namespace UStructProperty
	{
		inline int32 Struct;
	}

	namespace UArrayProperty
	{
		inline int32 Inner;
	}

	namespace UMapProperty
	{
		struct UMapPropertyBase
		{
			class UProperty* KeyProperty;
			class UProperty* ValueProperty;
		};

		inline int32 Base;
	}

	namespace USetProperty
	{
		inline int32 ElementProp;
	}

	namespace UEnumProperty
	{
		struct UEnumPropertyBase
		{
			class UProperty* UnderlayingProperty;
			class UEnum* Enum;
		};

		inline int32 Base;
	}
}