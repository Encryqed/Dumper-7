
#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Generators/EngineVersioning.h"
#include "Generators/MappingGenerator.h"
#include "Managers/PackageManager.h"
#include "Compression/zstd.h"

#include "../Settings.h"
#include "Utils.h"

namespace
{
	std::unordered_map<std::string, int32> GNameMap;

	std::vector<uint64> GFlagDict;
	std::unordered_map<uint64, uint32> GFlagIndexOf;
	bool GAnyMappingProperty = false;

	/*
	* 'PropertyCount' is written as an int24 whose top byte carries the Class/Struct flag: 1 for structs, 0 for
	* classes. Keeping both fields in one struct makes the packing explicit instead of leaving it spread over
	* shifts and masks at the write site.
	*/
	struct FUsmapPropertyCount
	{
		uint32 Count : 24 = 0x0;
		uint32 bIsStruct : 8 = 0x0;

		inline uint32 GetAsUint32() const
		{
			return static_cast<uint32>(Count) | (static_cast<uint32>(bIsStruct) << 24);
		}
	};
}


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
	else if (Flags & EClassCastFlags::Utf8StrProperty)
	{
		return EMappingsTypeFlags::Utf8StrProperty;
	}
	else if (Flags & EClassCastFlags::AnsiStrProperty)
	{
		return EMappingsTypeFlags::AnsiStrProperty;
	}
	else if (Flags & EClassCastFlags::ClassProperty)
	{
		return EMappingsTypeFlags::ClassProperty;
	}
	else if (Flags & EClassCastFlags::MulticastInlineDelegateProperty)
	{
		return EMappingsTypeFlags::MulticastInlineDelegateProperty;
	}
	else if (Flags & EClassCastFlags::SoftClassProperty)
	{
		return EMappingsTypeFlags::SoftClassProperty;
	}
	
	return EMappingsTypeFlags::Unknown;
}

int32 MappingGenerator::AddNameToData(std::stringstream& NameTable, const std::string& Name)
{
	if constexpr (Settings::MappingGenerator::bShouldCheckForDuplicatedNames)
	{
		auto [It, bInserted] = GNameMap.insert({ Name, static_cast<int32>(NameCounter) });

		/* The name didn't occure yet, write it to the NameTable */
		if (bInserted)
		{
			WriteToStream(NameTable, static_cast<uint16>(Name.length()));
			NameTable.write(Name.c_str(), Name.length());
			return static_cast<int32>(NameCounter++);
		}

		return It->second;
	}

	WriteToStream(NameTable, static_cast<uint16>(Name.length()));
	NameTable.write(Name.c_str(), Name.length());

	return static_cast<int32>(NameCounter++);
}

void MappingGenerator::WriteOwnerPackageName(const UEObject& Object, std::stringstream& Data, std::stringstream& NameTable)
{
	const UEObject Package = Object.GetOutermost();
	if (!Package)
	{
		WriteToStream(Data, -1);
		return;
	}

	WriteToStream(Data, AddNameToData(NameTable, Package.GetName()));
}

bool MappingGenerator::ShouldExcludeEditorOnlyProperties()
{
	/* ExtendedMetadata dumps must include editor-only properties */
	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
		return false;

	return Settings::MappingGenerator::bExcludeEditorOnlyProperties;
}

bool MappingGenerator::ShouldWriteMappingProperty(const PropertyWrapper& Property)
{
	if (!Property.IsUnrealProperty())
		return false;

	return !(ShouldExcludeEditorOnlyProperties() && Property.HasPropertyFlags(EPropertyFlags::EditorOnly));
}

void MappingGenerator::CollectPropertyFlags(const StructWrapper& Struct)
{
	if (!Struct.IsValid())
		return;

	MemberManager Members = Struct.GetMembers();
	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (!ShouldWriteMappingProperty(Member))
			continue;

		GAnyMappingProperty = true;

		const uint64 Flags = static_cast<uint64>(Member.GetPropertyFlags());
		if (GFlagIndexOf.emplace(Flags, static_cast<uint32>(GFlagDict.size())).second)
			GFlagDict.push_back(Flags);
	}
}

void MappingGenerator::CollectAllPropertyFlags()
{
	GFlagDict.clear();
	GFlagIndexOf.clear();
	GAnyMappingProperty = false;

	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		if (!Package.HasClasses() && !Package.HasStructs())
			continue;

		const DependencyManager::OnVisitCallbackType CollectFlagsCallback = [&](int32 Index) -> void
		{
			CollectPropertyFlags(ObjectArray::GetByIndex<UEStruct>(Index));
		};

		if (Package.HasStructs())
			Package.GetSortedStructs().VisitAllNodesWithCallback(CollectFlagsCallback);

		if (Package.HasClasses())
			Package.GetSortedClasses().VisitAllNodesWithCallback(CollectFlagsCallback);
	}

	if (GFlagDict.empty() && GAnyMappingProperty)
	{
		GFlagDict.push_back(0);
		GFlagIndexOf[0] = 0;
	}
}

void MappingGenerator::WriteFlagDictionary(std::stringstream& OutData)
{
	WriteToStream(OutData, static_cast<uint32>(GFlagDict.size()));
	for (const uint64 Flags : GFlagDict)
		WriteToStream(OutData, Flags);
}

void MappingGenerator::WriteUsmapMetadata(StreamType& InUsmap)
{
	WriteFUtf8String(InUsmap, MetadataToolName);
	WriteFUtf8String(InUsmap, MetadataToolVersion);

	const int64 CreatedAtUnix = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	WriteToStream(InUsmap, CreatedAtUnix);

	WriteToStream(InUsmap, static_cast<uint32>(MetadataSource));
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
		std::cerr << "\nInvalid non-Unreal property!\n" << std::endl;
		return;
	}

	WriteToStream(Data, static_cast<uint16>(Index));

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		WriteToStream(Data, static_cast<uint16>(Property.GetArrayDim()));
	}
	else
	{
		WriteToStream(Data, static_cast<uint8>(Property.GetArrayDim()));
	}

	const int32 MemberNameIdx = AddNameToData(NameTable, Property.GetUnrealProperty().GetName());
	WriteToStream(Data, MemberNameIdx);

	GeneratePropertyType(Property.GetUnrealProperty(), Data, NameTable);

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		const uint64 Flags = static_cast<uint64>(Property.GetPropertyFlags());
		const auto It = GFlagIndexOf.find(Flags);
		const uint16 FlagIndex = static_cast<uint16>((It != GFlagIndexOf.end()) ? It->second : 0);
		WriteToStream(Data, FlagIndex);
	}

	Index += Property.GetArrayDim();
}

void MappingGenerator::GenerateStruct(const StructWrapper& Struct, std::stringstream& Data, std::stringstream& NameTable)
{
	if (!Struct.IsValid())
		return;

	const int32 StructNameIndex = AddNameToData(NameTable, Struct.GetRawName());

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		if (Struct.IsUnrealStruct())
		{
			WriteOwnerPackageName(Struct.GetUnrealStruct(), Data, NameTable);
		}
		else
		{
			WriteToStream(Data, static_cast<int32>(-1));
		}
	}

	WriteToStream(Data, StructNameIndex);

	if (const StructWrapper Super = Struct.GetSuper(); Super.IsValid())
	{
		/* Most likely adds a duplicate to the name-table. Find a better solution later! */
		const int32 SuperNameIndex = AddNameToData(NameTable, Super.GetRawName());
		WriteToStream(Data, SuperNameIndex);
	}
	else
	{
		WriteToStream(Data, static_cast<int32>(-1));
	}

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		/* 'EClassFlags' for classes, 'EStructFlags' for structs. */
		uint32 Flags = 0x0;

		if (Struct.IsUnrealStruct())
		{
			const UEStruct UnrealStruct = Struct.GetUnrealStruct();

			Flags = Struct.IsClass()
				? static_cast<uint32>(UnrealStruct.Cast<UEClass>().GetClassFlags())
				: static_cast<uint32>(UnrealStruct.GetStructFlags());
		}

		WriteToStream(Data, Flags);
	}

	const MemberManager Members = Struct.GetMembers();

	uint32 PropertyCount = 0x0;
	uint32 SerializablePropertyCount = 0x0;
	const bool bExcludeEditorOnlyProps = ShouldExcludeEditorOnlyProperties();

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (bExcludeEditorOnlyProps && Member.HasPropertyFlags(EPropertyFlags::EditorOnly))
			continue;

		SerializablePropertyCount++;
		PropertyCount += Member.GetArrayDim();
	}

	FUsmapPropertyCount PropCount;
	PropCount.Count = PropertyCount;

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		/* 'PropertyCount' is int24; its top byte is the Class/Struct flag: 1 for structs, 0 for classes. */
		PropCount.bIsStruct = Struct.IsClass() ? 0x0 : 0x1;
	}

	/* uint32, uint32 */
	WriteToStream(Data, PropCount.GetAsUint32());
	WriteToStream(Data, SerializablePropertyCount);

	/* Incremented by 'Property->ArrayDim' inside 'GeneratePropertyInfo()' */
	int32 IndexIncrementedByFunction = 0x0;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (bExcludeEditorOnlyProps && Member.HasPropertyFlags(EPropertyFlags::EditorOnly))
			continue;

		GeneratePropertyInfo(Member, Data, NameTable, IndexIncrementedByFunction);
	}
}

void MappingGenerator::GenerateEnum(const EnumWrapper& Enum, std::stringstream& Data, std::stringstream& NameTable)
{
	const int32 EnumNameIndex = AddNameToData(NameTable, Enum.GetRawName());

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
		WriteOwnerPackageName(Enum.GetUnrealEnum(), Data, NameTable);

	WriteToStream(Data, EnumNameIndex);

	WriteToStream(Data, static_cast<uint16>(Enum.GetNumMembers()));

	for (EnumCollisionInfo Member : Enum.GetMembers())
	{
		const int32 EnumMemberNameIdx = AddNameToData(NameTable, Member.GetUniqueName());
		WriteToStream(Data, Member.GetValue());
		WriteToStream(Data, EnumMemberNameIdx);
	}
}

std::stringstream MappingGenerator::GenerateFileData()
{
	std::stringstream NameData;
	std::stringstream StructData;
	std::stringstream EnumData;

	uint32 NumEnums = 0x0;
	uint32 NumStructsAndClasses = 0x0;

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
	
	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
		CollectAllPropertyFlags();

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
			NumStructsAndClasses++;
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
		std::cerr << std::format("MappingGeneration: NameCounter = 0x{0:X} (Dec: {0})\n", static_cast<uint32>(NameCounter));

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
	{
		WriteFlagDictionary(ReturnBuffer);

		if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
			std::cerr << std::format("MappingGeneration: FlagDictCount = 0x{0:X} (Dec: {0})\n", static_cast<uint32>(GFlagDict.size()));
	}

	/* Write Enum-count and enums */
	WriteToStream(ReturnBuffer, NumEnums);
	WriteToStream(ReturnBuffer, EnumData);

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
		std::cerr << std::format("MappingGeneration: NumEnums = 0x{0:X} (Dec: {0})\n", static_cast<uint32>(NumEnums));

	/* Write Struct-count and structs */
	WriteToStream(ReturnBuffer, NumStructsAndClasses);
	WriteToStream(ReturnBuffer, StructData);

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
		std::cerr << std::format("MappingGeneration: NumStructsAndClasses = 0x{0:X} (Dec: {0})\n\n", static_cast<uint32>(NumStructsAndClasses));

	return ReturnBuffer;
}


void MappingGenerator::GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data)
{
	/* Write 2bytes unsigned */
	WriteToStream(InUsmap, UsmapFileMagic);

	/* Version: ExtendedMetadata */
	WriteToStream(InUsmap, WrittenVersion);

	if constexpr (WrittenVersion >= EUsmapVersion::ExtendedMetadata)
		WriteUsmapMetadata(InUsmap);

	/* PackageVersioning+: writes bHasVersioning and, when resolvable, the engine version. */
	if constexpr (WrittenVersion >= EUsmapVersion::PackageVersioning)
		EngineVersioning::WritePackageVersioning(InUsmap, WrittenVersion >= EUsmapVersion::EngineVersioning);

	/* Create a string_view to avoid expensive heap allocation each time we need.str().data() */
	std::string_view DataView = Data.view();
	const uint32 UncompressedSize = static_cast<uint32>(DataView.length());

	constexpr auto CompressionMethod = Settings::MappingGenerator::CompressionMethod;

	/* Write 'CompressionMethod' to the compression byte */
	WriteToStream(InUsmap, static_cast<uint8>(CompressionMethod));

	size_t CompressedSize = UncompressedSize;
	void* CompressedBuffer;

	switch (CompressionMethod)
	{
	case EUsmapCompressionMethod::ZStandard:
		CompressedSize = ZSTD_compressBound(UncompressedSize);
		CompressedBuffer = malloc(CompressedSize);
		CompressedSize = ZSTD_compress(CompressedBuffer, CompressedSize, DataView.data(), UncompressedSize, ZSTD_maxCLevel());
		break;
	default:
		CompressedBuffer = malloc(CompressedSize);
		memcpy(CompressedBuffer, DataView.data(), CompressedSize);
		break;
	}

	if constexpr (Settings::Debug::bShouldPrintMappingDebugData)
	{
		std::cerr << std::format("MappingGeneration: CompressedSize = 0x{0:X} (Dec: {0})\n", CompressedSize);
		std::cerr << std::format("MappingGeneration: DecompressedSize = 0x{0:X} (Dec: {0})\n\n", UncompressedSize);
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
	GNameMap.clear();
	GFlagDict.clear();
	GFlagIndexOf.clear();
	GAnyMappingProperty = false;

	NameCounter = 0x0;

	std::string MappingsFileName = (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName + ".usmap");

	FileNameHelper::MakeValidFileName(MappingsFileName);

	/* Open the stream as binary data, else ofstream will add \r after numbers that can be interpreted as \n. */
	std::ofstream UsmapFile(MainFolder / MappingsFileName, std::ios::binary);

	/* Generate the payload of the file, containing all of the names, enums and structs. */
	const std::stringstream FileData = GenerateFileData();

	/* Generate the header, and write both header and payload into the file. */
	GenerateFileHeader(UsmapFile, FileData);
}
