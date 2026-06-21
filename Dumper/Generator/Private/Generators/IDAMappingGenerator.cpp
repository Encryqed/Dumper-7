#include "Generators/IDAMappingGenerator.h"
#include "SharedPredefinedMembers.h"

#include "Managers/PackageManager.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

std::string IDAMappingGenerator::MangleFunctionName(const std::string& ClassName, const std::string& FunctionName)
{
	return "_ZN" + std::to_string(ClassName.length()) + ClassName + std::to_string(FunctionName.length()) + FunctionName + "Ev";
}

std::string IDAMappingGenerator::MangleUFunctionName(const std::string& ClassName, const std::string& FunctionName)
{
	return MangleFunctionName(ClassName, "exec" + FunctionName);
}

IDAMappingsLayouts::StringOffset IDAMappingGenerator::AddNameToData(std::stringstream& NameTable, const std::string& Name)
{
	if constexpr (Settings::MappingGenerator::bShouldCheckForDuplicatedNames)
	{
		static std::unordered_map<std::string, IDAMappingsLayouts::StringOffset> NameMap;

		const IDAMappingsLayouts::StringOffset CurrentOffset = static_cast<IDAMappingsLayouts::StringOffset>(NameTable.tellp());

		auto [It, bInserted] = NameMap.insert({ Name, CurrentOffset });

		if (bInserted)
		{
			WriteToStream(NameTable, static_cast<uint16>(Name.length()));
			NameTable.write(Name.c_str(), Name.length());
			return CurrentOffset;
		}

		return It->second;
	}

	const IDAMappingsLayouts::StringOffset CurrentOffset = static_cast<IDAMappingsLayouts::StringOffset>(NameTable.tellp());
	WriteToStream(NameTable, static_cast<uint16>(Name.length()));
	NameTable.write(Name.c_str(), Name.length());

	return CurrentOffset;
}

static std::string GetIDATypeFromSize(uint8 Size)
{
	switch (Size)
	{
	case 1:
		return "unsigned __int8";
	case 2:
		return "unsigned __int16";
	case 4:
		return "unsigned int";
	case 8:
		return "unsigned __int64";
	default:
		return "unsigned __int8";
	}
}

std::string IDAMappingGenerator::GetStructPrefixedName(const StructWrapper& Struct)
{
	if (Struct.IsFunction())
		return Struct.GetUnrealStruct().GetOuter().GetValidName() + "_" + Struct.GetName();

	auto [ValidName, bIsUnique] = Struct.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	return PackageManager::GetName(Struct.GetUnrealStruct().GetPackageIndex()) + "_" + ValidName;
}

std::string IDAMappingGenerator::GetEnumPrefixedName(const EnumWrapper& Enum)
{
	auto [ValidName, bIsUnique] = Enum.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	return PackageManager::GetName(Enum.GetUnrealEnum().GetPackageIndex()) + "_" + ValidName;
}

static std::string ConvertPredefinedTypeForIDA(std::string Type, bool& OutIsPtr)
{
	OutIsPtr = false;

	// Strip trailing pointer
	if (!Type.empty() && Type.back() == '*')
	{
		Type.pop_back();

		while (!Type.empty() && Type.back() == ' ')
			Type.pop_back();

		OutIsPtr = true;
	}

	// Replace "class " with "struct "
	if (Type.starts_with("class "))
		Type.replace(0, 6, "struct ");

	// Strip template arguments since IDA can't parse angle bracket syntax (or im just retarded?)
	auto AngleBracket = Type.find('<');
	if (AngleBracket != std::string::npos)
	{
		Type = Type.substr(0, AngleBracket);

		while (!Type.empty() && Type.back() == ' ')
			Type.pop_back();
	}

	if (Type == "uint8")						return "unsigned __int8";
	if (Type == "int8")							return "__int8";
	if (Type == "uint16")						return "unsigned __int16";
	if (Type == "int16")						return "__int16";
	if (Type == "int32")						return "int";
	if (Type == "uint32")						return "unsigned int";
	if (Type == "int64")						return "__int64";
	if (Type == "uint64")						return "unsigned __int64";

	// Strip "enum class " prefix
	if (Type.starts_with("enum class "))
		Type = Type.substr(11);

	// If it's not a basic type and has no struct/enum prefix, add struct prefix (but not for known enum types)
	if (!Type.starts_with("struct ") && !Type.starts_with("enum ") &&
		Type != "void" && Type != "float" && Type != "double" && Type != "bool" &&
		Type != "char" && Type != "wchar_t" &&
		Type != "EObjectFlags" && Type != "EClassCastFlags" && Type != "EFunctionFlags")
	{
		// Types like "TMap", "TArray" etc. after template stripping
		Type = "struct " + Type;
	}

	return Type;
}

std::string IDAMappingGenerator::GetIDACppType(const PropertyWrapper& Member, bool& OutIsPtr)
{
	if (!Member.IsUnrealProperty())
		return ConvertPredefinedTypeForIDA(Member.GetType(), OutIsPtr);

	return GetIDACppTypeForProperty(Member.GetUnrealProperty(), OutIsPtr);
}

std::string IDAMappingGenerator::GetIDACppTypeForProperty(UEProperty Property, bool& OutIsPtr)
{
	OutIsPtr = false;

	auto [Class, FieldClass] = Property.GetClass();
	EClassCastFlags Flags = Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();

	if (Flags & EClassCastFlags::ByteProperty)
	{
		if (UEEnum Enum = Property.Cast<UEByteProperty>().GetEnum())
			return GetEnumPrefixedName(Enum);

		return "unsigned __int8";
	}
	else if (Flags & EClassCastFlags::UInt16Property)
	{
		return "unsigned __int16";
	}
	else if (Flags & EClassCastFlags::UInt32Property)
	{
		return "unsigned int";
	}
	else if (Flags & EClassCastFlags::UInt64Property)
	{
		return "unsigned __int64";
	}
	else if (Flags & EClassCastFlags::Int8Property)
	{
		return "__int8";
	}
	else if (Flags & EClassCastFlags::Int16Property)
	{
		return "__int16";
	}
	else if (Flags & EClassCastFlags::IntProperty)
	{
		return "int";
	}
	else if (Flags & EClassCastFlags::Int64Property)
	{
		return "__int64";
	}
	else if (Flags & EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (Flags & EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (Flags & EClassCastFlags::ClassProperty)
	{
		OutIsPtr = true;
		return "struct UClass";
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return "struct FName";
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return "struct FString";
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return "struct FText";
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return Property.Cast<UEBoolProperty>().IsNativeBool() ? "bool" : GetIDATypeFromSize(Property.GetSize());
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		const StructWrapper& UnderlayingStruct = Property.Cast<UEStructProperty>().GetUnderlayingStruct();
		return std::format("struct {}", GetStructPrefixedName(UnderlayingStruct));
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return "struct TArray";
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		return "struct TWeakObjectPtr";
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		return "struct TLazyObjectPtr";
	}
	else if (Flags & EClassCastFlags::SoftClassProperty)
	{
		return "struct TSoftClassPtr";
	}
	else if (Flags & EClassCastFlags::SoftObjectProperty)
	{
		return "struct TSoftObjectPtr";
	}
	else if (Flags & EClassCastFlags::ObjectProperty)
	{
		OutIsPtr = true;

		if (UEClass PropertyClass = Property.Cast<UEObjectProperty>().GetPropertyClass())
			return std::format("struct {}", GetStructPrefixedName(PropertyClass));

		return "struct UObject";
	}
	else if (Settings::EngineCore::bEnableEncryptedObjectPropertySupport && (Flags & EClassCastFlags::ObjectPropertyBase))
	{
		return "struct TEncryptedObjPtr";
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		return "struct TMap";
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return "struct TSet";
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		if (UEEnum Enum = Property.Cast<UEEnumProperty>().GetEnum())
			return GetEnumPrefixedName(Enum);

		// Fallback: use underlying property's integer type
		bool bInnerIsPtr = false;
		return GetIDACppTypeForProperty(Property.Cast<UEEnumProperty>().GetUnderlayingProperty(), bInnerIsPtr);
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		return "struct TScriptInterface";
	}
	else if (Flags & EClassCastFlags::DelegateProperty)
	{
		return "struct FScriptDelegate";
	}
	else if (Flags & EClassCastFlags::MulticastInlineDelegateProperty)
	{
		return "struct FMulticastInlineDelegate";
	}
	else if (Flags & EClassCastFlags::FieldPathProperty)
	{
		if (Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty)
		{
			OutIsPtr = true;

			if (UEClass PropertyClass = Property.Cast<UEObjectProperty>().GetPropertyClass())
				return std::format("struct {}", GetStructPrefixedName(PropertyClass));

			return "struct UObject";
		}

		return "struct TFieldPath";
	}
	else if (Flags & EClassCastFlags::OptionalProperty)
	{
		return "struct TOptional";
	}
	else if (Flags & EClassCastFlags::Utf8StrProperty)
	{
		return "struct FUtf8String";
	}
	else if (Flags & EClassCastFlags::AnsiStrProperty)
	{
		return "struct FUtf8String";
	}
	else
	{
		return GetIDATypeFromSize(Property.GetSize());
	}
}

void IDAMappingGenerator::InitPredefinedMembers()
{
	InitCorePredefinedMembers(PredefinedMembers);

	for (auto& [Index, Predefs] : PredefinedMembers)
		std::sort(Predefs.Members.begin(), Predefs.Members.end(), ComparePredefinedMembers);
}

#define IDA_MEMBER_AT(IdaType, NameStr, OffsetExpr, SizeExpr, AlignExpr) \
	PredefinedMember{ \
		.Type = IdaType, .Name = NameStr, \
		.Offset = static_cast<int32>(OffsetExpr), \
		.Size = static_cast<int32>(SizeExpr), \
		.ArrayDim = 1, \
		.Alignment = static_cast<int32>(AlignExpr), \
		.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF \
	}

uint32 IDAMappingGenerator::GeneratePredefinedTypes(std::stringstream& StructData, std::stringstream& NameData)
{
	const int32 PtrSize = sizeof(void*);
	const int32 FNameSize = Off::InSDK::Name::FNameSize;
	const int32 FWeakObjectPtrSize = 0x08; // always two int32s

	std::vector<PredefinedStruct> Types;
	Types.reserve(32);

	auto AddPredef = [&](const char* Name, int32 Size, int32 Align) -> PredefinedStruct&
	{
		return Types.emplace_back(PredefinedStruct{
			.UniqueName = Name, .Size = Size, .Alignment = Align,
			.bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		});
	};

	// TArray<T>: { T* Data; int32 NumElements; int32 MaxElements; }
	{
		PredefinedStruct& T = AddPredef("TArray", sizeof(TArray<int>), alignof(TArray<int>));
		T.Properties = {
			IDA_MEMBER_AT("void*", "Data",        0,                              PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32", "NumElements", PtrSize,                        sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32", "MaxElements", PtrSize + (int32)sizeof(int32), sizeof(int32), alignof(int32)),
		};
	}

	// FString = TArray<wchar_t>
	{
		PredefinedStruct& T = AddPredef("FString", sizeof(TArray<int>), alignof(TArray<int>));
		T.Properties = {
			IDA_MEMBER_AT("wchar_t*", "Data",        0,                              PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32",    "NumElements", PtrSize,                        sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32",    "MaxElements", PtrSize + (int32)sizeof(int32), sizeof(int32), alignof(int32)),
		};
	}

	// FUtf8String = TArray<char>
	{
		PredefinedStruct& T = AddPredef("FUtf8String", sizeof(TArray<int>), alignof(TArray<int>));
		T.Properties = {
			IDA_MEMBER_AT("char*", "Data",        0,                              PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32", "NumElements", PtrSize,                        sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32", "MaxElements", PtrSize + (int32)sizeof(int32), sizeof(int32), alignof(int32)),
		};
	}

	// FName: layout depends on FNameSize (runtime)
	{
		PredefinedStruct& T = AddPredef("FName", FNameSize, alignof(int32));
		T.Properties.push_back(IDA_MEMBER_AT("int32", "ComparisonIndex", 0, sizeof(int32), alignof(int32)));

		if (FNameSize >= sizeof(int32) * 2)
			T.Properties.push_back(IDA_MEMBER_AT("int32", "Number", sizeof(int32), sizeof(int32), alignof(int32)));

		if (FNameSize >= sizeof(int32) * 3)
			T.Properties.push_back(IDA_MEMBER_AT("int32", "DisplayIndex", sizeof(int32) * 2, sizeof(int32), alignof(int32)));
	}

	// TWeakObjectPtr: { int32 ObjectIndex; int32 ObjectSerialNumber; }
	{
		PredefinedStruct& T = AddPredef("TWeakObjectPtr", FWeakObjectPtrSize, alignof(int32));
		T.Properties = {
			IDA_MEMBER_AT("int32", "ObjectIndex",        0,             sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32", "ObjectSerialNumber", sizeof(int32), sizeof(int32), alignof(int32)),
		};
	}

	// FScriptDelegate: { FWeakObjectPtr Object; FName FunctionName; } runtime FNameSize
	{
		PredefinedStruct& T = AddPredef("FScriptDelegate", PropertySizes::DelegateProperty, alignof(int32));
		T.Properties = {
			IDA_MEMBER_AT("struct TWeakObjectPtr", "Object",       0,                  FWeakObjectPtrSize, alignof(int32)),
			IDA_MEMBER_AT("struct FName",          "FunctionName", FWeakObjectPtrSize, FNameSize,          alignof(int32)),
		};
	}

	// FMulticastInlineDelegate: { TArray<FScriptDelegate> InvocationList; }
	{
		PredefinedStruct& T = AddPredef("FMulticastInlineDelegate", PropertySizes::MulticastInlineDelegateProperty, alignof(TArray<int>));
		T.Properties = {
			IDA_MEMBER_AT("struct TArray", "InvocationList", 0, sizeof(TArray<int>), alignof(TArray<int>)),
		};
	}

	// TScriptInterface: { UObject* ObjectPointer; void* InterfacePointer; }
	{
		PredefinedStruct& T = AddPredef("TScriptInterface", PtrSize * 2, alignof(void*));
		T.Properties = {
			IDA_MEMBER_AT("void*", "ObjectPointer",    0,       PtrSize, alignof(void*)),
			IDA_MEMBER_AT("void*", "InterfacePointer", PtrSize, PtrSize, alignof(void*)),
		};
	}

	// FGuid: { uint32 A, B, C, D; }
	{
		PredefinedStruct& T = AddPredef("FGuid", static_cast<int32>(sizeof(uint32) * 4), alignof(uint32));
		T.Properties = {
			IDA_MEMBER_AT("uint32", "A", sizeof(uint32) * 0, sizeof(uint32), alignof(uint32)),
			IDA_MEMBER_AT("uint32", "B", sizeof(uint32) * 1, sizeof(uint32), alignof(uint32)),
			IDA_MEMBER_AT("uint32", "C", sizeof(uint32) * 2, sizeof(uint32), alignof(uint32)),
			IDA_MEMBER_AT("uint32", "D", sizeof(uint32) * 3, sizeof(uint32), alignof(uint32)),
		};
	}

	// too lazy to properly impl TMap & TSet (TODO: do that pls?)
	AddPredef("TMap", sizeof(TMap<int, int>), alignof(TMap<int, int>));
	AddPredef("TSet", sizeof(TSet<int>),      alignof(TSet<int>));

	// TFieldPath: { FField* ResolvedField; TWeakObjectPtr<UStruct> ResolvedOwner; TArray<FName> Path; }
	{
		PredefinedStruct& T = AddPredef("TFieldPath", PropertySizes::FieldPathProperty, alignof(void*));
		T.Properties = {
			IDA_MEMBER_AT("struct FField*",        "ResolvedField", 0,                            PtrSize,             alignof(void*)),
			IDA_MEMBER_AT("struct TWeakObjectPtr", "ResolvedOwner", PtrSize,                      FWeakObjectPtrSize,  alignof(int32)),
			IDA_MEMBER_AT("struct TArray",         "Path",          PtrSize + FWeakObjectPtrSize, sizeof(TArray<int>), alignof(TArray<int>)),
		};
	}

	// FUObjectItem: { UObject* Object; ... padding to FUObjectItemSize } runtime offsets
	{
		PredefinedStruct& T = AddPredef("FUObjectItem", Off::InSDK::ObjArray::FUObjectItemSize, PtrSize);
		T.Properties = {
			IDA_MEMBER_AT("void*", "Object", Off::InSDK::ObjArray::FUObjectItemInitialOffset, PtrSize, alignof(void*)),
		};
	}

	// TUObjectArray: chunked vs fixed layout
	if (Off::InSDK::ObjArray::ChunkSize > 0)
	{
		const auto& Layout = Off::FUObjectArray::ChunkedFixedLayout;
		const int32 ObjectArraySize = (std::max)({ Layout.ObjectsOffset + PtrSize, Layout.MaxElementsOffset + (int32)sizeof(int32), Layout.NumElementsOffset + (int32)sizeof(int32), Layout.MaxChunksOffset + (int32)sizeof(int32), Layout.NumChunksOffset + (int32)sizeof(int32) });

		PredefinedStruct& T = AddPredef("TUObjectArray", ObjectArraySize, PtrSize);
		T.Properties = {
			IDA_MEMBER_AT("struct FUObjectItem**", "Objects",     Layout.ObjectsOffset,     PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32",                 "MaxElements", Layout.MaxElementsOffset, sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32",                 "NumElements", Layout.NumElementsOffset, sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32",                 "MaxChunks",   Layout.MaxChunksOffset,   sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32",                 "NumChunks",   Layout.NumChunksOffset,   sizeof(int32), alignof(int32)),
		};
	}
	else
	{
		const auto& Layout = Off::FUObjectArray::FixedLayout;
		const int32 ObjectArraySize = (std::max)({ Layout.ObjectsOffset + PtrSize, Layout.MaxObjectsOffset + (int32)sizeof(int32), Layout.NumObjectsOffset + (int32)sizeof(int32) });

		PredefinedStruct& T = AddPredef("TUObjectArray", ObjectArraySize, PtrSize);
		T.Properties = {
			IDA_MEMBER_AT("struct FUObjectItem*", "Objects",     Layout.ObjectsOffset,    PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32",                "MaxElements", Layout.MaxObjectsOffset, sizeof(int32), alignof(int32)),
			IDA_MEMBER_AT("int32",                "NumElements", Layout.NumObjectsOffset, sizeof(int32), alignof(int32)),
		};
	}

	// TNameEntryArray: only when not using FNamePool
	if (Off::InSDK::Name::AppendNameToString == 0x0 && !Settings::Internal::bUseNamePool)
	{
		const int32 ChunkTableSize = Off::NameArray::NumElements / PtrSize;
		const int32 ChunkTableSizeBytes = ChunkTableSize * PtrSize;
		const int32 NameArraySize = ChunkTableSizeBytes + PtrSize;

		PredefinedStruct& T = AddPredef("TNameEntryArray", NameArraySize, PtrSize);
		T.Properties = {
			IDA_MEMBER_AT("void*", "Chunks",      0,                                            ChunkTableSizeBytes, alignof(void*)),
			IDA_MEMBER_AT("int32", "NumElements", ChunkTableSizeBytes,                          sizeof(int32),       alignof(int32)),
			IDA_MEMBER_AT("int32", "NumChunks",   ChunkTableSizeBytes + (int32)sizeof(int32),   sizeof(int32),       alignof(int32)),
		};
	}

	// FStructBaseChain: void** (1 ptr) + int32 slot (uint32 + trailing pad)
	if (Off::UStruct::StructBaseChain != -1)
	{
		PredefinedStruct& T = AddPredef("FStructBaseChain", PtrSize * 2, alignof(void*));
		T.Properties = {
			IDA_MEMBER_AT("void**", "StructBaseChainArray",          0,       PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("int32",  "NumStructBasesInChainMinusOne", PtrSize, sizeof(int32), alignof(int32)),
		};
	}

	// FField: { void* VTable; FFieldClass* ClassPrivate; FFieldVariant Owner; FField* Next; FName Name; int32 ObjFlags; }
	if (Settings::Internal::bUseFProperty)
	{
		PredefinedStruct& T = AddPredef("FField", PtrSize * 4 + FNameSize + (int32)sizeof(int32), PtrSize);
		T.Properties = {
			IDA_MEMBER_AT("void*",          "VTable",       0,                       PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("void*",          "ClassPrivate", PtrSize,                 PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("void*",          "Owner",        PtrSize * 2,             PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("struct FField*", "Next",         PtrSize * 3,             PtrSize,       alignof(void*)),
			IDA_MEMBER_AT("struct FName",   "Name",         PtrSize * 4,             FNameSize,     alignof(int32)),
			IDA_MEMBER_AT("int32",          "ObjFlags",     PtrSize * 4 + FNameSize, sizeof(int32), alignof(int32)),
		};
	}

	// FText
	AddPredef("FText", Off::InSDK::Text::TextSize, alignof(void*));

	for (const PredefinedStruct& Predef : Types)
		GenerateSingleStruct(StructWrapper(&Predef), StructData, NameData);

	return static_cast<uint32_t>(Types.size());
}

#undef IDA_MEMBER_AT

void IDAMappingGenerator::GenerateSingleMember(const PropertyWrapper& Member, std::stringstream& StructData, std::stringstream& NameData, int32 StructSize)
{
	bool bIsPtr = false;
	const IDAMappingsLayouts::StringOffset TypeName = AddNameToData(NameData, GetIDACppType(Member, bIsPtr));
	const IDAMappingsLayouts::StringOffset Name = AddNameToData(NameData, Member.GetName());

	const uint8_t BitIndex = Member.IsBitField() ? Member.GetBitIndex() : 0xFF;

	const int32 MemberOffset = static_cast<int32>(Member.GetOffset());
	int32 MemberSize = static_cast<int32>(Member.GetSize());

	if (MemberOffset + MemberSize > StructSize)
		MemberSize = (std::max)(0, StructSize - MemberOffset);

	WriteToStream(StructData, TypeName);
	WriteToStream(StructData, Name);

	WriteToStream(StructData, MemberOffset);
	WriteToStream(StructData, MemberSize);
	WriteToStream(StructData, static_cast<int32>(Member.GetArrayDim()));

	WriteToStream(StructData, static_cast<bool>(bIsPtr));
	WriteToStream(StructData, static_cast<uint8_t>(BitIndex));
}

void IDAMappingGenerator::GenerateSingleStruct(const StructWrapper& Struct, std::stringstream& StructData, std::stringstream& NameData)
{
	const StructWrapper Super = Struct.GetSuper();

	const IDAMappingsLayouts::StringOffset Name = AddNameToData(NameData, Struct.GetUniqueName().first);
	const IDAMappingsLayouts::StringOffset SuperName = Super.IsValid() ? AddNameToData(NameData, Super.GetUniqueName().first) : static_cast<IDAMappingsLayouts::StringOffset>(-1);

	WriteToStream(StructData, Name);
	WriteToStream(StructData, SuperName);

	WriteToStream(StructData, static_cast<int32>(Struct.GetSize()));
	WriteToStream(StructData, static_cast<int32>(Struct.GetAlignment()));

	const MemberManager Members = Struct.GetMembers();

	int32 NumNonStaticMembers = 0;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (!Member.IsStatic() && !Member.IsZeroSizedMember())
			NumNonStaticMembers++;
	}

	WriteToStream(StructData, NumNonStaticMembers);

	const int32 StructSize = static_cast<int32>(Struct.GetSize());

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		if (Member.IsStatic() || Member.IsZeroSizedMember())
			continue;

		GenerateSingleMember(Member, StructData, NameData, StructSize);
	}
}

void IDAMappingGenerator::GenerateSingleEnum(const EnumWrapper& Enum, std::stringstream& EnumData, std::stringstream& NameData)
{
	const IDAMappingsLayouts::StringOffset Name = AddNameToData(NameData, Enum.GetUniqueName().first);

	WriteToStream(EnumData, Name);
	WriteToStream(EnumData, static_cast<uint8_t>(Enum.GetUnderlyingTypeSize()));
	WriteToStream(EnumData, static_cast<int32_t>(Enum.GetNumMembers()));

	for (EnumCollisionInfo Member : Enum.GetMembers())
	{
		const int32 MemberName = AddNameToData(NameData, Member.GetUniqueName());

		WriteToStream(EnumData, static_cast<IDAMappingsLayouts::StringOffset>(MemberName));
		WriteToStream(EnumData, static_cast<int64_t>(Member.GetValue()));
	}
}

void IDAMappingGenerator::GenerateVTableName(std::stringstream& ExecFuncData, std::stringstream& NameData, UEObject DefaultObject)
{
	const UEClass Class = DefaultObject.GetClass();
	const UEClass Super = Class.GetSuper().Cast<UEClass>();

	if (Super && DefaultObject.GetVft() == Super.GetDefaultObject().GetVft())
		return;

	const std::string Name = Class.GetCppName() + "_VFT";
	const IDAMappingsLayouts::StringOffset NameOffset = AddNameToData(NameData, Name);
	const uint32 Offset = static_cast<uint32>(Platform::GetOffset(DefaultObject.GetVft()));

	WriteToStream(ExecFuncData, NameOffset);
	WriteToStream(ExecFuncData, Offset);
}

void IDAMappingGenerator::GenerateClassFunctions(std::stringstream& ExecFuncData, std::stringstream& NameData, UEClass Class)
{
	static std::unordered_map<uint32, std::string> Funcs;

	for (const UEFunction Func : Class.GetFunctions())
	{
		if (!Func.HasFlags(EFunctionFlags::Native))
			continue;

		const std::string MangledName = MangleUFunctionName(Class.GetCppName(), Func.GetValidName());
		const uint32 Offset = static_cast<uint32>(Platform::GetOffset(Func.GetExecFunction()));

		auto [It, bInserted] = Funcs.emplace(Offset, Func.GetFullName());

		if (!bInserted)
			continue;

		const IDAMappingsLayouts::StringOffset NameOffset = AddNameToData(NameData, MangledName);

		WriteToStream(ExecFuncData, NameOffset);
		WriteToStream(ExecFuncData, Offset);
	}
}

uint32 IDAMappingGenerator::GenerateInternalEnums(std::stringstream& EnumData, std::stringstream& NameData)
{
	struct EnumValueDef { const char* Name; int64_t Value; };
	struct EnumDef { const char* Name; uint8_t UnderlyingSize; std::vector<EnumValueDef> Values; };

	#define E(EnumType, Name) { #Name, static_cast<int64_t>(EnumType::Name) }

	std::vector<EnumDef> Enums;

	Enums.push_back({ "EObjectFlags", sizeof(EObjectFlags), {
		E(EObjectFlags, NoFlags), E(EObjectFlags, Public), E(EObjectFlags, Standalone),
		E(EObjectFlags, MarkAsNative), E(EObjectFlags, Transactional), E(EObjectFlags, ClassDefaultObject),
		E(EObjectFlags, ArchetypeObject), E(EObjectFlags, Transient), E(EObjectFlags, MarkAsRootSet),
		E(EObjectFlags, TagGarbageTemp), E(EObjectFlags, NeedInitialization), E(EObjectFlags, NeedLoad),
		E(EObjectFlags, KeepForCooker), E(EObjectFlags, NeedPostLoad), E(EObjectFlags, NeedPostLoadSubobjects),
		E(EObjectFlags, NewerVersionExists), E(EObjectFlags, BeginDestroyed), E(EObjectFlags, FinishDestroyed),
		E(EObjectFlags, BeingRegenerated), E(EObjectFlags, DefaultSubObject), E(EObjectFlags, WasLoaded),
		E(EObjectFlags, TextExportTransient), E(EObjectFlags, LoadCompleted),
		E(EObjectFlags, InheritableComponentTemplate), E(EObjectFlags, DuplicateTransient),
		E(EObjectFlags, StrongRefOnFrame), E(EObjectFlags, NonPIEDuplicateTransient),
		E(EObjectFlags, Dynamic), E(EObjectFlags, WillBeLoaded),
	}});

	Enums.push_back({ "EFunctionFlags", sizeof(EFunctionFlags), {
		E(EFunctionFlags, None), E(EFunctionFlags, Final), E(EFunctionFlags, RequiredAPI),
		E(EFunctionFlags, BlueprintAuthorityOnly), E(EFunctionFlags, BlueprintCosmetic),
		E(EFunctionFlags, Net), E(EFunctionFlags, NetReliable), E(EFunctionFlags, NetRequest),
		E(EFunctionFlags, Exec), E(EFunctionFlags, Native), E(EFunctionFlags, Event),
		E(EFunctionFlags, NetResponse), E(EFunctionFlags, Static), E(EFunctionFlags, NetMulticast),
		E(EFunctionFlags, UbergraphFunction), E(EFunctionFlags, MulticastDelegate),
		E(EFunctionFlags, Public), E(EFunctionFlags, Private), E(EFunctionFlags, Protected),
		E(EFunctionFlags, Delegate), E(EFunctionFlags, NetServer), E(EFunctionFlags, HasOutParms),
		E(EFunctionFlags, HasDefaults), E(EFunctionFlags, NetClient), E(EFunctionFlags, DLLImport),
		E(EFunctionFlags, BlueprintCallable), E(EFunctionFlags, BlueprintEvent),
		E(EFunctionFlags, BlueprintPure), E(EFunctionFlags, EditorOnly),
		E(EFunctionFlags, Const), E(EFunctionFlags, NetValidate), E(EFunctionFlags, AllFlags),
	}});

	Enums.push_back({ "EClassCastFlags", sizeof(EClassCastFlags), {
		E(EClassCastFlags, None), E(EClassCastFlags, Field), E(EClassCastFlags, Int8Property),
		E(EClassCastFlags, Enum), E(EClassCastFlags, Struct), E(EClassCastFlags, ScriptStruct),
		E(EClassCastFlags, Class), E(EClassCastFlags, ByteProperty), E(EClassCastFlags, IntProperty),
		E(EClassCastFlags, FloatProperty), E(EClassCastFlags, UInt64Property),
		E(EClassCastFlags, ClassProperty), E(EClassCastFlags, UInt32Property),
		E(EClassCastFlags, InterfaceProperty), E(EClassCastFlags, NameProperty),
		E(EClassCastFlags, StrProperty), E(EClassCastFlags, Property),
		E(EClassCastFlags, ObjectProperty), E(EClassCastFlags, BoolProperty),
		E(EClassCastFlags, UInt16Property), E(EClassCastFlags, Function),
		E(EClassCastFlags, StructProperty), E(EClassCastFlags, ArrayProperty),
		E(EClassCastFlags, Int64Property), E(EClassCastFlags, DelegateProperty),
		E(EClassCastFlags, NumericProperty), E(EClassCastFlags, MulticastDelegateProperty),
		E(EClassCastFlags, ObjectPropertyBase), E(EClassCastFlags, WeakObjectProperty),
		E(EClassCastFlags, LazyObjectProperty), E(EClassCastFlags, SoftObjectProperty),
		E(EClassCastFlags, TextProperty), E(EClassCastFlags, Int16Property),
		E(EClassCastFlags, DoubleProperty), E(EClassCastFlags, SoftClassProperty),
		E(EClassCastFlags, Package), E(EClassCastFlags, Level),
		E(EClassCastFlags, Actor), E(EClassCastFlags, PlayerController),
		E(EClassCastFlags, Pawn), E(EClassCastFlags, SceneComponent),
		E(EClassCastFlags, PrimitiveComponent), E(EClassCastFlags, SkinnedMeshComponent),
		E(EClassCastFlags, SkeletalMeshComponent), E(EClassCastFlags, Blueprint),
		E(EClassCastFlags, DelegateFunction), E(EClassCastFlags, StaticMeshComponent),
		E(EClassCastFlags, MapProperty), E(EClassCastFlags, SetProperty),
		E(EClassCastFlags, EnumProperty), E(EClassCastFlags, SparseDelegateFunction),
		E(EClassCastFlags, MulticastInlineDelegateProperty),
		E(EClassCastFlags, MulticastSparseDelegateProperty),
		E(EClassCastFlags, FieldPathProperty),
	}});

	#undef E

	for (const auto& Enum : Enums)
	{
		IDAMappingsLayouts::StringOffset NameOffset = AddNameToData(NameData, Enum.Name);
		WriteToStream(EnumData, NameOffset);
		WriteToStream(EnumData, Enum.UnderlyingSize);
		WriteToStream(EnumData, static_cast<int32_t>(Enum.Values.size()));

		for (const auto& Value : Enum.Values)
		{
			IDAMappingsLayouts::StringOffset ValueName = AddNameToData(NameData, Value.Name);
			WriteToStream(EnumData, ValueName);
			WriteToStream(EnumData, Value.Value);
		}
	}

	return static_cast<uint32_t>(Enums.size());
}

void IDAMappingGenerator::Generate()
{
	std::stringstream NameData;
	std::stringstream EnumData;
	std::stringstream StructData;
	std::stringstream ExecFuncData;
	std::stringstream NamedVarData;

	uint32_t NumEnums = 0;
	uint32_t NumStructs = 0;

	// Generate internal engine enums (EObjectFlags, EFunctionFlags, EClassCastFlags) before reflected enums
	NumEnums += GenerateInternalEnums(EnumData, NameData);

	// Generate known UE types (TArray, FString, FName, etc.) before reflected structs
	NumStructs += GeneratePredefinedTypes(StructData, NameData);

	// Generate enums and structs from packages
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		if (Package.HasEnums())
		{
			for (int32 EnumIdx : Package.GetEnums())
			{
				GenerateSingleEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx), EnumData, NameData);
				NumEnums++;
			}
		}

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			Structs.VisitAllNodesWithCallback([&](int32 Index) -> void
			{
				GenerateSingleStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructData, NameData);
				NumStructs++;
			});
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();

			Classes.VisitAllNodesWithCallback([&](int32 Index) -> void
			{
				GenerateSingleStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructData, NameData);
				NumStructs++;
			});
		}
	}

	// Generate exec functions (VTables + class functions)
	for (UEObject Obj : ObjectArray())
	{
		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject))
		{
			GenerateVTableName(ExecFuncData, NameData, Obj);
		}
		else if (Obj.IsA(EClassCastFlags::Class))
		{
			GenerateClassFunctions(ExecFuncData, NameData, Obj.Cast<UEClass>());
		}
	}

	// Generate named variables (GObjects, GNames)
	uint32_t NumGlobalSymbols = 0;

	auto WriteNamedVar = [&](IDAMappingsLayouts::OffsetType VarOffset, const std::string& TypeStr, const std::string& NameStr)
	{
		IDAMappingsLayouts::NamedVariable Var{};
		Var.VariableOffset = VarOffset;
		Var.Type = AddNameToData(NameData, TypeStr);
		Var.Name = AddNameToData(NameData, NameStr);
		WriteToStream(NamedVarData, Var);
		NumGlobalSymbols++;
	};

	WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::ObjArray::GObjects), "TUObjectArray", "GObjects");
	WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::NameArray::GNames), "TNameEntryArray", "GNames");

	if (Off::InSDK::World::GWorld != 0x0)
		WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::World::GWorld), "UWorld*", "GWorld");

	if (Off::InSDK::ProcessEvent::PEOffset != 0x0)
		WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::ProcessEvent::PEOffset), "void*", "UObject::ProcessEvent");

	if (Off::InSDK::Name::AppendNameToString != 0x0)
	{
		const char* FuncName = Off::InSDK::Name::bIsUsingAppendStringOverToString ? "FName::AppendString" : "FName::ToString";
		WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::Name::AppendNameToString), "void*", FuncName);
	}

	if (Off::InSDK::Name::GetNameEntryFromName != 0x0)
		WriteNamedVar(static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::Name::GetNameEntryFromName), "void*", "FName::GetNameEntryFromName");

	// Get section data as strings
	const std::string NameDataStr = NameData.str();
	const std::string EnumDataStr = EnumData.str();
	const std::string StructDataStr = StructData.str();
	const std::string ExecFuncDataStr = ExecFuncData.str();
	const std::string NamedVarDataStr = NamedVarData.str();

	const uint32_t NumExecFunctions = static_cast<uint32_t>(ExecFuncDataStr.size() / sizeof(IDAMappingsLayouts::ExecFunc));

	// Build header with section offsets
	IDAMappingsLayouts::IDAMappingsHeader Header{};
	Header.Magic = IDAMappingsLayouts::FileMagic;
	Header.Version = IDAMappingsLayouts::EIDAMappingsVersion::Initial;
	Header.Reserved = 0;

	uint32_t CurrentOffset = sizeof(IDAMappingsLayouts::IDAMappingsHeader);

	Header.StringDataSizeBytes = static_cast<uint32_t>(NameDataStr.size());
	Header.StringDataOffset = CurrentOffset;
	CurrentOffset += Header.StringDataSizeBytes;

	Header.NumEnums = NumEnums;
	Header.EnumDataOffset = CurrentOffset;
	CurrentOffset += static_cast<uint32_t>(EnumDataStr.size());

	Header.NumStructs = NumStructs;
	Header.StructDataOffset = CurrentOffset;
	CurrentOffset += static_cast<uint32_t>(StructDataStr.size());

	Header.NumGlobalSymbols = NumGlobalSymbols;
	Header.GlobalSymbolDataOffset = CurrentOffset;
	CurrentOffset += static_cast<uint32_t>(NamedVarDataStr.size());

	Header.NumExecFunctions = NumExecFunctions;
	Header.ExecFunctionDataOffset = CurrentOffset;

	// Write to file
	std::string MappingsFileName = (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName + ".idmap");

	FileNameHelper::MakeValidFileName(MappingsFileName);

	StreamType IDAMappingsFile(MainFolder / MappingsFileName, std::ios::binary);

	// Header
	IDAMappingsFile.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

	// Sections in order: Strings, Enums, Structs, GlobalSymbols, ExecFunctions
	IDAMappingsFile.write(NameDataStr.data(), NameDataStr.size());
	IDAMappingsFile.write(EnumDataStr.data(), EnumDataStr.size());
	IDAMappingsFile.write(StructDataStr.data(), StructDataStr.size());
	IDAMappingsFile.write(NamedVarDataStr.data(), NamedVarDataStr.size());
	IDAMappingsFile.write(ExecFuncDataStr.data(), ExecFuncDataStr.size());
}
