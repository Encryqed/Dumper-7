
#include <iostream>
#include <string>

#include "Generators/MappingGenerator.h"
#include "Managers/PackageManager.h"
#include "Compression/zstd.h"

#include "../Settings.h"
#include "Utils.h"

EMappingsTypeFlags MappingGenerator::GetMappingType(UEProperty Property)
{
	auto [Class, FieldClass] = Property.GetClass();

	EClassCastFlags Flags = Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();

	if (Flags & EClassCastFlags::ByteProperty)
	{
		return EMappingsTypeFlags::ByteProperty;
	}
	else if (Flags & EClassCastFlags::UInt16Property)
	{
		return EMappingsTypeFlags::UInt16Property;
	}
	else if (Flags & EClassCastFlags::UInt32Property)
	{
		return EMappingsTypeFlags::UInt32Property;
	}
	else if (Flags & EClassCastFlags::UInt64Property)
	{
		return EMappingsTypeFlags::UInt64Property;
	}
	else if (Flags & EClassCastFlags::Int8Property)
	{
		return EMappingsTypeFlags::Int8Property;
	}
	else if (Flags & EClassCastFlags::Int16Property)
	{
		return EMappingsTypeFlags::Int16Property;
	}
	else if (Flags & EClassCastFlags::IntProperty)
	{
		return EMappingsTypeFlags::IntProperty;
	}
	else if (Flags & EClassCastFlags::Int64Property)
	{
		return EMappingsTypeFlags::Int64Property;
	}
	else if (Flags & EClassCastFlags::FloatProperty)
	{
		return EMappingsTypeFlags::FloatProperty;
	}
	else if (Flags & EClassCastFlags::DoubleProperty)
	{
		return EMappingsTypeFlags::DoubleProperty;
	}
	else if ((Flags & EClassCastFlags::ObjectProperty) || (Flags & EClassCastFlags::ClassProperty))
	{
		return EMappingsTypeFlags::ObjectProperty;
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return EMappingsTypeFlags::NameProperty;
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return EMappingsTypeFlags::StrProperty;
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return EMappingsTypeFlags::TextProperty;
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return EMappingsTypeFlags::BoolProperty;
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		return EMappingsTypeFlags::StructProperty;
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return EMappingsTypeFlags::ArrayProperty;
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		return EMappingsTypeFlags::WeakObjectProperty;
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		return EMappingsTypeFlags::LazyObjectProperty;
	}
	else if ((Flags & EClassCastFlags::SoftObjectProperty) || (Flags & EClassCastFlags::SoftClassProperty))
	{
		return EMappingsTypeFlags::SoftObjectProperty;
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		return EMappingsTypeFlags::MapProperty;
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return EMappingsTypeFlags::SetProperty;
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		return EMappingsTypeFlags::EnumProperty;
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		return EMappingsTypeFlags::InterfaceProperty;
	}
	else if (Flags & EClassCastFlags::FieldPathProperty)
	{
		return EMappingsTypeFlags::FieldPathProperty;
	}
	else if (Flags & EClassCastFlags::OptionalProperty)
	{
		return EMappingsTypeFlags::OptionalProperty;
	}
	else if (Flags & EClassCastFlags::MulticastDelegateProperty)
	{
		return EMappingsTypeFlags::MulticastDelegateProperty;
	}
	else if (Flags & EClassCastFlags::DelegateProperty)
	{
		return EMappingsTypeFlags::DelegateProperty;
	}
	
	return EMappingsTypeFlags::Unknown;
}

int32 MappingGenerator::AddNameToData(std::stringstream& NameTable, const std::string& Name)
{
	if constexpr (Settings::MappingGenerator::bShouldCheckForDuplicatedNames)
	{
		static std::unordered_map<std::string, int32> NameMap;
		
		auto [It, bInserted] = NameMap.insert({ Name, NameCounter });

		/* The name didn't occure yet, write it to the NameTable */
		if (bInserted)
		{
			WriteToStream(NameTable, static_cast<uint16>(Name.length()));
			NameTable.write(Name.c_str(), Name.length());
			return NameCounter++;
		}

		return It->second;
	}

	WriteToStream(NameTable, static_cast<uint16>(Name.length()));
	NameTable.write(Name.c_str(), Name.length());

	return NameCounter++;
}

void MappingGenerator::GeneratePropertyType(UEProperty Property, std::stringstream& Data, std::stringstream& NameTable)
{
	if (!Property)
	{
		WriteToStream(Data, static_cast<uint8>(EMappingsTypeFlags::Unknown));
		return;
	}

	EMappingsTypeFlags MappingType = GetMappingType(Property);

	/* Serialize ByteProperty as an EnumProperty with 'UnderlayingType == uint8' if the inner enum is valid */
	const bool bIsFakeEnumProperty = MappingType == EMappingsTypeFlags::ByteProperty && Property.Cast<UEByteProperty>().GetEnum();

	WriteToStream(Data, static_cast<uint8>(!bIsFakeEnumProperty ? MappingType : EMappingsTypeFlags::EnumProperty));

	/* Write ByteProperty as the fake EnumProperty's underlaying type */
	if (bIsFakeEnumProperty)
		WriteToStream(Data, static_cast<uint8>(EMappingsTypeFlags::ByteProperty));

	if (MappingType == EMappingsTypeFlags::EnumProperty)
	{
		GeneratePropertyType(Property.Cast<UEEnumProperty>().GetUnderlayingProperty(), Data, NameTable);

		const int32 EnumNameIdx = AddNameToData(NameTable, Property.Cast<UEEnumProperty>().GetEnum().GetName());
		WriteToStream(Data, EnumNameIdx);
	}
	else if (bIsFakeEnumProperty)
	{
		const int32 EnumNameIdx = AddNameToData(NameTable, Property.Cast<UEByteProperty>().GetEnum().GetName());
		WriteToStream(Data, EnumNameIdx);
	}
	else if (MappingType == EMappingsTypeFlags::StructProperty)
	{
		const int32 StructNameIdx = AddNameToData(NameTable, Property.Cast<UEStructProperty>().GetUnderlayingStruct().GetName());
		WriteToStream(Data, StructNameIdx);
	}
	else if (MappingType == EMappingsTypeFlags::SetProperty)
	{
		GeneratePropertyType(Property.Cast<UESetProperty>().GetElementProperty(), Data, NameTable);
	}
	else if (MappingType == EMappingsTypeFlags::ArrayProperty)
	{
		GeneratePropertyType(Property.Cast<UEArrayProperty>().GetInnerProperty(), Data, NameTable);
	}
	else if (MappingType == EMappingsTypeFlags::OptionalProperty)
	{
		GeneratePropertyType(Property.Cast<UEOptionalProperty>().GetValueProperty(), Data, NameTable);
	}
	else if (MappingType == EMappingsTypeFlags::MapProperty)
	{
		UEMapProperty AsMapProperty = Property.Cast<UEMapProperty>();
		GeneratePropertyType(AsMapProperty.GetKeyProperty(), Data, NameTable);
		GeneratePropertyType(AsMapProperty.GetValueProperty(), Data, NameTable);
	}
}

void MappingGenerator::GeneratePropertyInfo(const PropertyWrapper& Property, std::stringstream& Data, std::stringstream& NameTable, int32& Index)
{
	if (!Property.IsUnrealProperty())
	{
		std::cout << "\nInvalid non-Unreal property!\n" << std::endl;
		return;
	}

	WriteToStream(Data, static_cast<uint16>(Index));
	WriteToStream(Data, static_cast<uint8>(Property.GetArrayDim()));

	const int32 MemberNameIdx = AddNameToData(NameTable, Property.GetUnrealProperty().GetName());
	WriteToStream(Data, MemberNameIdx);

	GeneratePropertyType(Property.GetUnrealProperty(), Data, NameTable);

	Index += Property.GetArrayDim();
}

void MappingGenerator::GenerateStruct(const StructWrapper& Struct, std::stringstream& Data, std::stringstream& NameTable)
{
	if (!Struct.IsValid())
		return;

	const int32 StructNameIndex = AddNameToData(NameTable, Struct.GetRawName());
	WriteToStream(Data, StructNameIndex);

	StructWrapper Super = Struct.GetSuper();

	if (Super.IsValid())
	{
		/* Most likely adds a duplicate to the name-table. Find a better solution later! */
		const int32 SuperNameIndex = AddNameToData(NameTable, Super.GetRawName());
		WriteToStream(Data, SuperNameIndex);
	}
	else
	{
		WriteToStream(Data, static_cast<int32>(-1));
	}

	MemberManager Members = Struct.GetMembers();

	uint16 PropertyCount = 0x0;
	uint16 SerializablePropertyCount = 0x0;
	constexpr auto ExcludeEditorOnlyProps = Settings::MappingGenerator::bExcludeEditorOnlyProperties;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (ExcludeEditorOnlyProps && Member.HasPropertyFlags(EPropertyFlags::EditorOnly))
			continue;

		SerializablePropertyCount++;
		PropertyCount += Member.GetArrayDim();
	}

	/* uint16, uint16 */
	WriteToStream(Data, PropertyCount);
	WriteToStream(Data, SerializablePropertyCount);

	/* Incremented by 'Property->ArrayDim' inside 'GeneratePropertyInfo()' */
	int32 IndexIncrementedByFunction = 0x0;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (ExcludeEditorOnlyProps && Member.HasPropertyFlags(EPropertyFlags::EditorOnly))
			continue;

		GeneratePropertyInfo(Member, Data, NameTable, IndexIncrementedByFunction);
	}
}

void MappingGenerator::GenerateEnum(const EnumWrapper& Enum, std::stringstream& Data, std::stringstream& NameTable)
{
	const int32 EnumNameIndex = AddNameToData(NameTable, Enum.GetRawName());
	WriteToStream(Data, EnumNameIndex);

	WriteToStream(Data, static_cast<uint16>(Enum.GetNumMembers()));

	for (EnumCollisionInfo Member : Enum.GetMembers())
	{
		const int32 EnumMemberNameIdx = AddNameToData(NameTable, Member.GetUniqueName());
		WriteToStream(Data, EnumMemberNameIdx);
	}
}


std::stringstream MappingGenerator::GenerateFileData()
{
	std::stringstream NameData;
	std::stringstream StructData;
	std::stringstream EnumData;

	uint32 NumEnums = 0x0;
	uint32 NumStructsAndClasse = 0x0;

	/* Handle all Enums first */
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		/* Create files and handles namespaces and includes */
		if (!Package.HasEnums())
			continue;

		for (int32 EnumIdx : Package.GetEnums())
		{
			GenerateEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx), EnumData, NameData);
			NumEnums++;
		}
	}
	
	/* Handle all structs and classes in one go. From the mapping-files point of view classes are the exact same as structs. */
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		/* Create files and handles namespaces and includes */
		if (!Package.HasClasses() && !Package.HasStructs())
			continue;

		DependencyManager::OnVisitCallbackType GenerateStructCallback = [&](int32 Index) -> void
		{
			GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructData, NameData);
			NumStructsAndClasse++;
		};

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();
			Structs.VisitAllNodesWithCallback(GenerateStructCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();
			Classes.VisitAllNodesWithCallback(GenerateStructCallback);
		}
	}

	/* Combine all of the stringstreams into one Data block representing the entire payload of the file */
	std::stringstream ReturnBuffer;

	/* Write Name-count and names */
	WriteToStream(ReturnBuffer, static_cast<uint32>(NameCounter));
	WriteToStream(ReturnBuffer, NameData);

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
		std::cout << std::format("MappingGeneration: NameCounter = 0x{0:X} (Dec: {0})\n", static_cast<uint32>(NameCounter));

	/* Write Enum-count and enums */
	WriteToStream(ReturnBuffer, static_cast<uint32>(NumEnums));
	WriteToStream(ReturnBuffer, EnumData);

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
		std::cout << std::format("MappingGeneration: NumEnums = 0x{0:X} (Dec: {0})\n", static_cast<uint32>(NumEnums));

	/* Write Struct-count and enums */
	WriteToStream(ReturnBuffer, static_cast<uint32>(NumStructsAndClasse));
	WriteToStream(ReturnBuffer, StructData);

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
		std::cout << std::format("MappingGeneration: NumStructsAndClasse = 0x{0:X} (Dec: {0})\n\n", static_cast<uint32>(NumStructsAndClasse));

	return ReturnBuffer;
}


void MappingGenerator::GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data)
{
	/* Write 2bytes unsigned */
	WriteToStream(InUsmap, UsmapFileMagic);

	/* Version: LargeEnums, some games contain enums exceeding 255 values */
	WriteToStream(InUsmap, EUsmapVersion::LargeEnums);

	/* We're on 'LargeEnums' version, we need to write 'bool' (aka int32) bHasVersioning. (NoVersioning = false) -> no [int32 UE4Version, int32 UE5Version] and no [uint32 NetCL] */
	WriteToStream(InUsmap, static_cast<int32>(false));

	const uint32 UncompressedSize = static_cast<uint32>(Data.str().length());

	constexpr auto CompressionMethod = Settings::MappingGenerator::CompressionMethod;

	/* Write 'CompressionMethod' to the compression byte */
	WriteToStream(InUsmap, static_cast<uint8>(CompressionMethod));

	size_t CompressedSize = UncompressedSize;
	void* CompressedBuffer = nullptr;

	switch (CompressionMethod)
	{
	case EUsmapCompressionMethod::ZStandard:
		CompressedSize = ZSTD_compressBound(UncompressedSize);
		CompressedBuffer = malloc(CompressedSize);
		CompressedSize = ZSTD_compress(CompressedBuffer, CompressedSize, Data.str().data(), UncompressedSize, ZSTD_maxCLevel());
		break;
	default:
		CompressedBuffer = malloc(CompressedSize);
		memcpy(CompressedBuffer, Data.str().data(), CompressedSize);
		break;
	}

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
	{
		std::cout << std::format("MappingGeneration: CompressedSize = 0x{0:X} (Dec: {0})\n", CompressedSize);
		std::cout << std::format("MappingGeneration: DecompressedSize = 0x{0:X} (Dec: {0})\n\n", UncompressedSize);
	}

	/* Write compressed size */
	WriteToStream(InUsmap, static_cast<uint32>(CompressedSize));

	/* Write uncompressed size */
	WriteToStream(InUsmap, UncompressedSize);

	/* Header is done, now write the payload to the file */
	InUsmap.write(static_cast<const char*>(CompressedBuffer), static_cast<uint32>(CompressedSize));

	free(CompressedBuffer);
}

void MappingGenerator::Generate()
{
	NameCounter = 0x0;

	std::string MappingsFileName = (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName + ".usmap");

	FileNameHelper::MakeValidFileName(MappingsFileName);

	/* Open the stream as binary data, else ofstream will add \r after numbers that can be interpreted as \n. */
	std::ofstream UsmapFile(MainFolder / MappingsFileName, std::ios::binary);

	/* Generate the payload of the file, containing all of the names, enums and structs. */
	std::stringstream FileData = GenerateFileData();

	/* Generate the header, and write both header and payload into the file. */
	GenerateFileHeader(UsmapFile, FileData);
}

