#pragma once
#include "Enums.h"
#include "Settings.h"

namespace Off
{
	void Init();

	//Offsets not to be used during generation but inside of the generated SDK
	namespace InSDK
	{
		inline int32 PEIndex;
		inline int32 PEOffset;
		inline int32 GObjects;
		inline int32 ChunkSize;
		inline int32 FUObjectItemSize;
		inline int32 AppendNameToString;
	}

	namespace FUObjectArray
	{
		inline uint32 Num;
	}

	namespace UObject
	{
		inline constexpr const uint32 Vft = 0x00;
		inline constexpr const uint32 Flags = 0x08;
		inline constexpr const uint32 Index = 0x0C;
		inline constexpr const uint32 Class = 0x10;
		inline constexpr const uint32 Name = 0x18;
	
#ifndef WITH_CASE_PRESERVING_NAME
	inline constexpr const uint32 Outer = 0x18 + 0x08;
#else
	inline constexpr const uint32 Outer = 0x18 + 0x10;
#endif // WITH_CASEPRESERVING_NAME

}

namespace UField
{
#ifndef WITH_CASE_PRESERVING_NAME
	inline constexpr const uint32 Next = 0x18 + 0x10;
#else
	inline constexpr const uint32 Next = 0x18 + 0x18;
#endif // WITH_CASEPRESERVING_NAME
}
	namespace UEnum
	{
		inline uint32 Names;
	}

	namespace UStruct
	{
		inline uint32 SuperStruct;
		inline uint32 Children;
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