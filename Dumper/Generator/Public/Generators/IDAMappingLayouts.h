#pragma once

#include <cstdint>

namespace IDAMappingsLayouts
{
	using StringOffset = uint32_t;
	using OffsetType = uint32_t;
	using InternalOffset = uint32_t;

	static constexpr uint8_t FileMagic = 0xD7;

	static constexpr StringOffset InvalidStringOffset = 0xFFFFFFFF;

#pragma pack(push, 1)

	struct Member
	{
		StringOffset Type;
		StringOffset Name;

		int32_t Offset;
		int32_t Size;
		int32_t ArrayDim;
		bool bIsPointer;
		uint8_t BitFieldBitCount; // 0xFF by default if this isn't a bitfield
	};

	struct Struct
	{
		StringOffset Name;
		StringOffset SuperName;

		int32_t Size;
		int32_t Alignment;

		int32_t NumMembers;
		Member Members[1];
	};

	struct EnumValue
	{
		StringOffset Name;
		int64_t Value;
	};

	struct Enum
	{
		StringOffset Name;
		uint8_t UnderlyingTypeSizeBytes;

		int32_t NumValues;
		EnumValue Values[1];
	};

	struct ExecFunc
	{
		StringOffset MangledName;
		OffsetType OffsetRelativeToImagebase;

		StringOffset CppTypeSignature; // Use this one when the SDK was imported with clang
		StringOffset FallbackCppSignatureInfo; // Fallback for manual import
	};


	struct NamedVariable
	{
		OffsetType VariableOffset;

		StringOffset Type;
		StringOffset Name;
	};

	struct NamedVTable
	{
		OffsetType VTableOffset;
		OffsetType SuperVTableOffset;
		StringOffset Name;
	};

	struct StringData
	{
		uint16_t StringLength;
		const char Utf8StringData[1];
	};

	enum class EIDAMappingsVersion : uint8_t
	{
		Initial = 1,
		WithExecSignatures = 2
	};

	struct IDAMappingsHeader
	{
		uint8_t Magic;
		EIDAMappingsVersion Version;
		uint8_t Reserved;

		uint32_t StringDataSizeBytes;
		InternalOffset StringDataOffset;

		uint32_t NumEnums;
		InternalOffset EnumDataOffset;

		uint32_t NumStructs;
		InternalOffset StructDataOffset;

		uint32_t NumGlobalSymbols;
		InternalOffset GlobalSymbolDataOffset;

		uint32_t NumVTables;
		InternalOffset VTableDataOffset;

		uint32_t NumExecFunctions;
		InternalOffset ExecFunctionDataOffset;

		InternalOffset ExecFuncSignatureDataOffset;
	};

#pragma pack(pop)

}
