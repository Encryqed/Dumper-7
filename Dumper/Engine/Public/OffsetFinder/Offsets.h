#pragma once
#include "Unreal/Enums.h"
#include "../Settings.h"


struct FFixedUObjectArrayLayout
{
	int32 ObjectsOffset = -1;
	int32 MaxObjectsOffset = -1;
	int32 NumObjectsOffset = -1;

	inline bool IsValid() const
	{
		return ObjectsOffset != -1 && MaxObjectsOffset != -1 && NumObjectsOffset != -1;
	}
};

struct FChunkedFixedUObjectArrayLayout
{
	int32 ObjectsOffset = -1;
	int32 MaxElementsOffset = -1;
	int32 NumElementsOffset = -1;
	int32 MaxChunksOffset = -1;
	int32 NumChunksOffset = -1;

	inline bool IsValid() const
	{
		return ObjectsOffset != -1 && MaxElementsOffset != -1 && NumElementsOffset != -1 && MaxChunksOffset != -1 && NumChunksOffset != -1;
	}
};

namespace Off
{
	void Init();

	//Offsets not to be used during generation but inside of the generated SDK
	namespace InSDK
	{
		namespace ProcessEvent
		{
			inline int32 PEIndex;
			inline int32 PEOffset;

			void InitPE();
			void InitPE(int32 Index, const char* const ModuleName = nullptr);
		}

		namespace World
		{
			inline int32 GWorld = 0x0;

			void InitGWorld();
		}

		namespace ObjArray
		{
			inline int32 GObjects;
			inline int32 ChunkSize;
			inline int32 FUObjectItemSize;
			inline int32 FUObjectItemInitialOffset;
		}

		namespace Name
		{
			/* Whether we're using FName::AppendString or, in an edge case, FName::ToString */
			inline bool bIsUsingAppendStringOverToString = true;
			inline int32 AppendNameToString;
			inline int32 FNameSize;
		}

		namespace NameArray
		{
			inline int32 GNames = 0x0;
			inline int32 FNamePoolBlockOffsetBits = 0x0;
			inline int32 FNameEntryStride = 0x0;
		}

		namespace Properties
		{
			inline int32 PropertySize;
		}

		namespace Text
		{
			inline int32 TextDatOffset = 0x0;

			inline int32 InTextDataStringOffset = 0x0;

			inline int32 TextSize = 0x0;

			void InitTextOffsets();
		}

		namespace ULevel
		{
			inline int32 Actors;
		}

		namespace UDataTable
		{
			inline int32 RowMap;
		}
	}

	namespace FUObjectArray
	{
		inline FFixedUObjectArrayLayout FixedLayout;
		inline FChunkedFixedUObjectArrayLayout ChunkedFixedLayout;

		inline bool bIsChunked = false;

		inline int32 GetObjectsOffset() { return  bIsChunked ? ChunkedFixedLayout.ObjectsOffset : FixedLayout.ObjectsOffset; }
		inline int32 GetNumElementsOffset() { return  bIsChunked ? ChunkedFixedLayout.NumElementsOffset : FixedLayout.NumObjectsOffset; }
	}

	namespace NameArray
	{
		inline int32 ChunksStart;
		inline int32 MaxChunkIndex;
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
		// Fixed for OutlineNumber FNames by OffsetFinder::FixFNameSize();
		inline int32 Name = 0x00;
		inline int32 Id = 0x08;
		inline int32 CastFlags = 0x10;
		inline int32 ClassFlags = 0x18;
		inline int32 SuperClass = 0x20;
	}

	namespace FName
	{
		// These values initialized by OffsetFinder::InitUObjectOffsets()
		inline int32 CompIdx = 0x0;
		inline int32 Number = 0x4;
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
		inline int32 MinAlignemnt;
	}

	namespace UFunction
	{
		inline int32 FunctionFlags;
		inline int32 ExecFunction;
	}

	namespace UClass
	{
		inline int32 CastFlags;
		inline int32 ClassDefaultObject;
	}

	namespace Property
	{
		inline int32 ArrayDim;
		inline int32 ElementSize;
		inline int32 PropertyFlags;
		inline int32 Offset_Internal;
	}

	namespace ByteProperty
	{
		inline int32 Enum;
	}

	namespace BoolProperty
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

	namespace ObjectProperty
	{
		inline int32 PropertyClass;
	}

	namespace ClassProperty
	{
		inline int32 MetaClass;
	}

	namespace StructProperty
	{
		inline int32 Struct;
	}

	namespace ArrayProperty
	{
		inline int32 Inner;
	}

	namespace DelegateProperty
	{
		inline int32 SignatureFunction;
	}

	namespace MapProperty
	{
		struct UMapPropertyBase
		{
			void* KeyProperty;
			void* ValueProperty;
		};

		inline int32 Base;
	}

	namespace SetProperty
	{
		inline int32 ElementProp;
	}

	namespace EnumProperty
	{
		struct UEnumPropertyBase
		{
			void* UnderlayingProperty;
			class UEnum* Enum;
		};

		inline int32 Base;
	}

	namespace FieldPathProperty
	{
		inline int32 FieldClass;
	}

	namespace OptionalProperty
	{
		inline int32 ValueProperty;
	}
}

namespace PropertySizes
{
	void Init();

	/* These are properties for which their size might change depending on the UE version or compilerflags. */
	inline int32 DelegateProperty = 0x10;
	void InitTDelegateSize();

	inline int32 FieldPathProperty = 0x20;
	void InitFFieldPathSize();
}
