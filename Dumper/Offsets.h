#pragma once
#include "Enums.h"

namespace Off
{
	inline void Init();

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
		inline constexpr const uint32 Outer = 0x20;
	}

	namespace UField
	{
		inline constexpr const uint32 Next = 0x28;
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
		inline uint8 ClassFlags;
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