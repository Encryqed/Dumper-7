#pragma once

#include <cstdint>
#include <type_traits>
#include <sstream>
#include <vector>

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

#define ENUM_OPERATORS(EEnumClass)																																		\
																																										\
inline constexpr EEnumClass operator|(EEnumClass Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass)((std::underlying_type<EEnumClass>::type)(Left) | (std::underlying_type<EEnumClass>::type)(Right));												\
}																																										\
																																										\
inline bool operator&(EEnumClass Left, EEnumClass Right)																												\
{																																										\
	return (((std::underlying_type<EEnumClass>::type)(Left) & (std::underlying_type<EEnumClass>::type)(Right)) == (std::underlying_type<EEnumClass>::type)(Right));		\
}																																										
																																																																																		
enum class EPropertyFlags : uint64
{
	None							= 0x0000000000000000,

	Edit							= 0x0000000000000001,
	ConstParm						= 0x0000000000000002,
	BlueprintVisible				= 0x0000000000000004,
	ExportObject					= 0x0000000000000008,
	BlueprintReadOnly				= 0x0000000000000010,
	Net								= 0x0000000000000020,
	EditFixedSize					= 0x0000000000000040,
	Parm							= 0x0000000000000080,
	OutParm							= 0x0000000000000100,
	ZeroConstructor					= 0x0000000000000200,
	ReturnParm						= 0x0000000000000400,
	DisableEditOnTemplate 			= 0x0000000000000800,

	Transient						= 0x0000000000002000,
	Config							= 0x0000000000004000,

	DisableEditOnInstance			= 0x0000000000010000,
	EditConst						= 0x0000000000020000,
	GlobalConfig					= 0x0000000000040000,
	InstancedReference				= 0x0000000000080000,	

	DuplicateTransient				= 0x0000000000200000,	
	SubobjectReference				= 0x0000000000400000,	

	SaveGame						= 0x0000000001000000,
	NoClear							= 0x0000000002000000,

	ReferenceParm					= 0x0000000008000000,
	BlueprintAssignable				= 0x0000000010000000,
	Deprecated						= 0x0000000020000000,
	IsPlainOldData					= 0x0000000040000000,
	RepSkip							= 0x0000000080000000,
	RepNotify						= 0x0000000100000000, 
	Interp							= 0x0000000200000000,
	NonTransactional				= 0x0000000400000000,
	EditorOnly						= 0x0000000800000000,
	NoDestructor					= 0x0000001000000000,

	AutoWeak						= 0x0000004000000000,
	ContainsInstancedReference		= 0x0000008000000000,	
	AssetRegistrySearchable			= 0x0000010000000000,
	SimpleDisplay					= 0x0000020000000000,
	AdvancedDisplay					= 0x0000040000000000,
	Protected						= 0x0000080000000000,
	BlueprintCallable				= 0x0000100000000000,
	BlueprintAuthorityOnly			= 0x0000200000000000,
	TextExportTransient				= 0x0000400000000000,
	NonPIEDuplicateTransient		= 0x0000800000000000,
	ExposeOnSpawn					= 0x0001000000000000,
	PersistentInstance				= 0x0002000000000000,
	UObjectWrapper					= 0x0004000000000000, 
	HasGetValueTypeHash				= 0x0008000000000000, 
	NativeAccessSpecifierPublic		= 0x0010000000000000,	
	NativeAccessSpecifierProtected	= 0x0020000000000000,
	NativeAccessSpecifierPrivate	= 0x0040000000000000,	
	SkipSerialization				= 0x0080000000000000, 
};

enum class EFunctionFlags : uint32
{
	None					= 0x00000000,

	Final					= 0x00000001,
	RequiredAPI				= 0x00000002,
	BlueprintAuthorityOnly	= 0x00000004, 
	BlueprintCosmetic		= 0x00000008, 
	Net						= 0x00000040,  
	NetReliable				= 0x00000080, 
	NetRequest				= 0x00000100,	
	Exec					= 0x00000200,	
	Native					= 0x00000400,	
	Event					= 0x00000800,   
	NetResponse				= 0x00001000,  
	Static					= 0x00002000,   
	NetMulticast			= 0x00004000,	
	UbergraphFunction		= 0x00008000,  
	MulticastDelegate		= 0x00010000,
	Public					= 0x00020000,	
	Private					= 0x00040000,	
	Protected				= 0x00080000,
	Delegate				= 0x00100000,	
	NetServer				= 0x00200000,	
	HasOutParms				= 0x00400000,	
	HasDefaults				= 0x00800000,
	NetClient				= 0x01000000,
	DLLImport				= 0x02000000,
	BlueprintCallable		= 0x04000000,
	BlueprintEvent			= 0x08000000,
	BlueprintPure			= 0x10000000,	
	EditorOnly				= 0x20000000,	
	Const					= 0x40000000,
	NetValidate				= 0x80000000,

	AllFlags = 0xFFFFFFFF,
};

enum class EObjectFlags
{
	NoFlags							= 0x00000000,

	Public							= 0x00000001,
	Standalone						= 0x00000002,
	MarkAsNative					= 0x00000004,
	Transactional					= 0x00000008,
	ClassDefaultObject				= 0x00000010,
	ArchetypeObject					= 0x00000020,
	Transient						= 0x00000040,

	MarkAsRootSet					= 0x00000080,
	TagGarbageTemp					= 0x00000100,

	NeedInitialization				= 0x00000200,
	NeedLoad						= 0x00000400,
	KeepForCooker					= 0x00000800,
	NeedPostLoad					= 0x00001000,
	NeedPostLoadSubobjects			= 0x00002000,
	NewerVersionExists				= 0x00004000,
	BeginDestroyed					= 0x00008000,
	FinishDestroyed					= 0x00010000,

	BeingRegenerated				= 0x00020000,
	DefaultSubObject				= 0x00040000,
	WasLoaded						= 0x00080000,
	TextExportTransient				= 0x00100000,
	LoadCompleted					= 0x00200000,
	InheritableComponentTemplate	= 0x00400000,
	DuplicateTransient				= 0x00800000,
	StrongRefOnFrame				= 0x01000000,
	NonPIEDuplicateTransient		= 0x02000000,
	Dynamic							= 0x04000000,
	WillBeLoaded					= 0x08000000, 
};

enum class EFieldClassID : uint64
{
	Int8					= 1llu << 1,
	Byte					= 1llu << 6,
	Int						= 1llu << 7,
	Float					= 1llu << 8,
	UInt64					= 1llu << 9,
	Class					= 1llu << 10,
	UInt32					= 1llu << 11,
	Interface				= 1llu << 12,
	Name					= 1llu << 13,
	String					= 1llu << 14,
	Object					= 1llu << 16,
	Bool					= 1llu << 17,
	UInt16					= 1llu << 18,
	Struct					= 1llu << 20,
	Array					= 1llu << 21,
	Int64					= 1llu << 22,
	Delegate				= 1llu << 23,
	SoftObject				= 1llu << 27,
	LazyObject				= 1llu << 28,
	WeakObject				= 1llu << 29,
	Text					= 1llu << 30,
	Int16					= 1llu << 31,
	Double					= 1llu << 32,
	SoftClass				= 1llu << 33,
	Map						= 1llu << 46,
	Set						= 1llu << 47,
	Enum					= 1llu << 48,
	MulticastInlineDelegate = 1llu << 50,
	MulticastSparseDelegate = 1llu << 51,
	ObjectPointer			= 1llu << 53
};

enum class EClassCastFlags : uint64
{
	None								= 0x0000000000000000,

	Field								= 0x0000000000000001,
	Int8Property						= 0x0000000000000002,
	Enum								= 0x0000000000000004,
	Struct								= 0x0000000000000008,
	ScriptStruct						= 0x0000000000000010,
	Class								= 0x0000000000000020,
	ByteProperty						= 0x0000000000000040,
	IntProperty							= 0x0000000000000080,
	FloatProperty						= 0x0000000000000100,
	UInt64Property						= 0x0000000000000200,
	ClassProperty						= 0x0000000000000400,
	UInt32Property						= 0x0000000000000800,
	InterfaceProperty					= 0x0000000000001000,
	NameProperty						= 0x0000000000002000,
	StrProperty							= 0x0000000000004000,
	Property							= 0x0000000000008000,
	ObjectProperty						= 0x0000000000010000,
	BoolProperty						= 0x0000000000020000,
	UInt16Property						= 0x0000000000040000,
	Function							= 0x0000000000080000,
	StructProperty						= 0x0000000000100000,
	ArrayProperty						= 0x0000000000200000,
	Int64Property						= 0x0000000000400000,
	DelegateProperty					= 0x0000000000800000,
	NumericProperty						= 0x0000000001000000,
	MulticastDelegateProperty			= 0x0000000002000000,
	ObjectPropertyBase					= 0x0000000004000000,
	WeakObjectProperty					= 0x0000000008000000,
	LazyObjectProperty					= 0x0000000010000000,
	SoftObjectProperty					= 0x0000000020000000,
	TextProperty						= 0x0000000040000000,
	Int16Property						= 0x0000000080000000,
	DoubleProperty						= 0x0000000100000000,
	SoftClassProperty					= 0x0000000200000000,
	Package								= 0x0000000400000000,
	Level								= 0x0000000800000000,
	Actor								= 0x0000001000000000,
	PlayerController					= 0x0000002000000000,
	Pawn								= 0x0000004000000000,
	SceneComponent						= 0x0000008000000000,
	PrimitiveComponent					= 0x0000010000000000,
	SkinnedMeshComponent				= 0x0000020000000000,
	SkeletalMeshComponent				= 0x0000040000000000,
	Blueprint							= 0x0000080000000000,
	DelegateFunction					= 0x0000100000000000,
	StaticMeshComponent					= 0x0000200000000000,
	MapProperty							= 0x0000400000000000,
	SetProperty							= 0x0000800000000000,
	EnumProperty						= 0x0001000000000000,
	SparseDelegateFunction				= 0x0002000000000000,
	MulticastInlineDelegateProperty		= 0x0004000000000000,
	MulticastSparseDelegateProperty		= 0x0008000000000000,
	FieldPathProperty					= 0x0010000000000000,
	ObjectPtrProperty					= 0x0020000000000000,
	ClassPtrProperty					= 0x0040000000000000,
	LargeWorldCoordinatesRealProperty	= 0x0080000000000000,
};

enum class EClassFlags
{
	CLASS_None					= 0x00000000u,
	Abstract					= 0x00000001u,
	DefaultConfig				= 0x00000002u,
	Config						= 0x00000004u,
	Transient					= 0x00000008u,
	Parsed						= 0x00000010u,
	MatchedSerializers			= 0x00000020u,
	ProjectUserConfig			= 0x00000040u,
	Native						= 0x00000080u,
	NoExport					= 0x00000100u,
	NotPlaceable				= 0x00000200u,
	PerObjectConfig				= 0x00000400u,
	ReplicationDataIsSetUp		= 0x00000800u,
	EditInlineNew				= 0x00001000u,
	CollapseCategories			= 0x00002000u,
	Interface					= 0x00004000u,
	CustomConstructor			= 0x00008000u,
	Const						= 0x00010000u,
	LayoutChanging				= 0x00020000u,
	CompiledFromBlueprint		= 0x00040000u,
	MinimalAPI					= 0x00080000u,
	RequiredAPI					= 0x00100000u,
	DefaultToInstanced			= 0x00200000u,
	TokenStreamAssembled		= 0x00400000u,
	HasInstancedReference		= 0x00800000u,
	Hidden						= 0x01000000u,
	Deprecated					= 0x02000000u,
	HideDropDown				= 0x04000000u,
	GlobalUserConfig			= 0x08000000u,
	Intrinsic					= 0x10000000u,
	Constructed					= 0x20000000u,
	ConfigDoNotCheckDefaults	= 0x40000000u,
	NewerVersionExists			= 0x80000000u,
};

enum class EMappingsTypeFlags : uint8
{
	ByteProperty,
	BoolProperty,
	IntProperty,
	FloatProperty,
	ObjectProperty,
	NameProperty,
	DelegateProperty,
	DoubleProperty,
	ArrayProperty,
	StructProperty,
	StrProperty,
	TextProperty,
	InterfaceProperty,
	MulticastDelegateProperty,
	WeakObjectProperty, //
	LazyObjectProperty, // When deserialized, these 3 properties will be SoftObjects
	AssetObjectProperty, //
	SoftObjectProperty,
	UInt64Property,
	UInt32Property,
	UInt16Property,
	Int64Property,
	Int16Property,
	Int8Property,
	MapProperty,
	SetProperty,
	EnumProperty,
	FieldPathProperty,

	Unknown = 0xFF
};

ENUM_OPERATORS(EObjectFlags);
ENUM_OPERATORS(EFunctionFlags);
ENUM_OPERATORS(EPropertyFlags);
ENUM_OPERATORS(EClassCastFlags);
ENUM_OPERATORS(EClassFlags);
ENUM_OPERATORS(EMappingsTypeFlags);
ENUM_OPERATORS(EFieldClassID);

static std::string StringifyFunctionFlags(EFunctionFlags FunctionFlags)
{
	std::vector<const char*> Buffer;

	if (FunctionFlags & EFunctionFlags::Final) { Buffer.push_back("Final"); }
	if (FunctionFlags & EFunctionFlags::RequiredAPI) { Buffer.push_back("RequiredAPI"); }
	if (FunctionFlags & EFunctionFlags::BlueprintAuthorityOnly) { Buffer.push_back("BlueprintAuthorityOnly"); }
	if (FunctionFlags & EFunctionFlags::BlueprintCosmetic) { Buffer.push_back("BlueprintCosmetic"); }
	if (FunctionFlags & EFunctionFlags::Net) { Buffer.push_back("Net"); }
	if (FunctionFlags & EFunctionFlags::NetReliable) { Buffer.push_back("NetReliable"); }
	if (FunctionFlags & EFunctionFlags::NetRequest) { Buffer.push_back("NetRequest"); }
	if (FunctionFlags & EFunctionFlags::Exec) { Buffer.push_back("Exec"); }
	if (FunctionFlags & EFunctionFlags::Native) { Buffer.push_back("Native"); }
	if (FunctionFlags & EFunctionFlags::Event) { Buffer.push_back("Event"); }
	if (FunctionFlags & EFunctionFlags::NetResponse) { Buffer.push_back("NetResponse"); }
	if (FunctionFlags & EFunctionFlags::Static) { Buffer.push_back("Static"); }
	if (FunctionFlags & EFunctionFlags::NetMulticast) { Buffer.push_back("NetMulticast"); }
	if (FunctionFlags & EFunctionFlags::UbergraphFunction) { Buffer.push_back("UbergraphFunction"); }
	if (FunctionFlags & EFunctionFlags::MulticastDelegate) { Buffer.push_back("MulticastDelegate"); }
	if (FunctionFlags & EFunctionFlags::Public) { Buffer.push_back("Public"); }
	if (FunctionFlags & EFunctionFlags::Private) { Buffer.push_back("Private"); }
	if (FunctionFlags & EFunctionFlags::Protected) { Buffer.push_back("Protected"); }
	if (FunctionFlags & EFunctionFlags::Delegate) { Buffer.push_back("Delegate"); }
	if (FunctionFlags & EFunctionFlags::NetServer) { Buffer.push_back("NetServer"); }
	if (FunctionFlags & EFunctionFlags::HasOutParms) { Buffer.push_back("HasOutParams"); }
	if (FunctionFlags & EFunctionFlags::HasDefaults) { Buffer.push_back("HasDefaults"); }
	if (FunctionFlags & EFunctionFlags::NetClient) { Buffer.push_back("NetClient"); }
	if (FunctionFlags & EFunctionFlags::DLLImport) { Buffer.push_back("DLLImport"); }
	if (FunctionFlags & EFunctionFlags::BlueprintCallable) { Buffer.push_back("BlueprintCallable"); }
	if (FunctionFlags & EFunctionFlags::BlueprintEvent) { Buffer.push_back("BlueprintEvent"); }
	if (FunctionFlags & EFunctionFlags::BlueprintPure) { Buffer.push_back("BlueprintPure"); }
	if (FunctionFlags & EFunctionFlags::EditorOnly) { Buffer.push_back("EditorOnly"); }
	if (FunctionFlags & EFunctionFlags::Const) { Buffer.push_back("Const"); }
	if (FunctionFlags & EFunctionFlags::NetValidate) { Buffer.push_back("NetValidate"); }

	switch (Buffer.size())
	{
	case 0:
		return std::string("None");
		break;
	case 1:
		return std::string(Buffer[0]);
		break;
	default:
		std::ostringstream os;
		std::copy(Buffer.begin(), Buffer.end() - 1, std::ostream_iterator<const char*>(os, ", "));
		os << *Buffer.rbegin();
		return os.str();
	}
}

static std::string StringifyPropertyFlags(EPropertyFlags PropertyFlags)
{
	std::vector<const char*> Buffer;

	if (PropertyFlags & EPropertyFlags::Edit) { Buffer.push_back("Edit"); }
	if (PropertyFlags & EPropertyFlags::ConstParm) { Buffer.push_back("ConstParm"); }
	if (PropertyFlags & EPropertyFlags::BlueprintVisible) { Buffer.push_back("BlueprintVisible"); }
	if (PropertyFlags & EPropertyFlags::ExportObject) { Buffer.push_back("ExportObject"); }
	if (PropertyFlags & EPropertyFlags::BlueprintReadOnly) { Buffer.push_back("BlueprintReadOnly"); }
	if (PropertyFlags & EPropertyFlags::Net) { Buffer.push_back("Net"); }
	if (PropertyFlags & EPropertyFlags::EditFixedSize) { Buffer.push_back("EditFixedSize"); }
	if (PropertyFlags & EPropertyFlags::Parm) { Buffer.push_back("Parm"); }
	if (PropertyFlags & EPropertyFlags::OutParm) { Buffer.push_back("OutParm"); }
	if (PropertyFlags & EPropertyFlags::ZeroConstructor) { Buffer.push_back("ZeroConstructor"); }
	if (PropertyFlags & EPropertyFlags::ReturnParm) { Buffer.push_back("ReturnParm"); }
	if (PropertyFlags & EPropertyFlags::DisableEditOnTemplate) { Buffer.push_back("DisableEditOnTemplate"); }
	if (PropertyFlags & EPropertyFlags::Transient) { Buffer.push_back("Transient"); }
	if (PropertyFlags & EPropertyFlags::Config) { Buffer.push_back("Config"); }
	if (PropertyFlags & EPropertyFlags::DisableEditOnInstance) { Buffer.push_back("DisableEditOnInstance"); }
	if (PropertyFlags & EPropertyFlags::EditConst) { Buffer.push_back("EditConst"); }
	if (PropertyFlags & EPropertyFlags::GlobalConfig) { Buffer.push_back("GlobalConfig"); }
	if (PropertyFlags & EPropertyFlags::InstancedReference) { Buffer.push_back("InstancedReference"); }
	if (PropertyFlags & EPropertyFlags::DuplicateTransient) { Buffer.push_back("DuplicateTransient"); }
	if (PropertyFlags & EPropertyFlags::SubobjectReference) { Buffer.push_back("SubobjectReference"); }
	if (PropertyFlags & EPropertyFlags::SaveGame) { Buffer.push_back("SaveGame"); }
	if (PropertyFlags & EPropertyFlags::NoClear) { Buffer.push_back("NoClear"); }
	if (PropertyFlags & EPropertyFlags::ReferenceParm) { Buffer.push_back("ReferenceParm"); }
	if (PropertyFlags & EPropertyFlags::BlueprintAssignable) { Buffer.push_back("BlueprintAssignable"); }
	if (PropertyFlags & EPropertyFlags::Deprecated) { Buffer.push_back("Deprecated"); }
	if (PropertyFlags & EPropertyFlags::IsPlainOldData) { Buffer.push_back("IsPlainOldData"); }
	if (PropertyFlags & EPropertyFlags::RepSkip) { Buffer.push_back("RepSkip"); }
	if (PropertyFlags & EPropertyFlags::RepNotify) { Buffer.push_back("RepNotify"); }
	if (PropertyFlags & EPropertyFlags::Interp) { Buffer.push_back("Interp"); }
	if (PropertyFlags & EPropertyFlags::NonTransactional) { Buffer.push_back("NonTransactional"); }
	if (PropertyFlags & EPropertyFlags::EditorOnly) { Buffer.push_back("EditorOnly"); }
	if (PropertyFlags & EPropertyFlags::NoDestructor) { Buffer.push_back("NoDestructor"); }
	if (PropertyFlags & EPropertyFlags::AutoWeak) { Buffer.push_back("AutoWeak"); }
	if (PropertyFlags & EPropertyFlags::ContainsInstancedReference) { Buffer.push_back("ContainsInstancedReference"); }
	if (PropertyFlags & EPropertyFlags::AssetRegistrySearchable) { Buffer.push_back("AssetRegistrySearchable"); }
	if (PropertyFlags & EPropertyFlags::SimpleDisplay) { Buffer.push_back("SimpleDisplay"); }
	if (PropertyFlags & EPropertyFlags::AdvancedDisplay) { Buffer.push_back("AdvancedDisplay"); }
	if (PropertyFlags & EPropertyFlags::Protected) { Buffer.push_back("Protected"); }
	if (PropertyFlags & EPropertyFlags::BlueprintCallable) { Buffer.push_back("BlueprintCallable"); }
	if (PropertyFlags & EPropertyFlags::BlueprintAuthorityOnly) { Buffer.push_back("BlueprintAuthorityOnly"); }
	if (PropertyFlags & EPropertyFlags::TextExportTransient) { Buffer.push_back("TextExportTransient"); }
	if (PropertyFlags & EPropertyFlags::NonPIEDuplicateTransient) { Buffer.push_back("NonPIEDuplicateTransient"); }
	if (PropertyFlags & EPropertyFlags::ExposeOnSpawn) { Buffer.push_back("ExposeOnSpawn"); }
	if (PropertyFlags & EPropertyFlags::PersistentInstance) { Buffer.push_back("PersistentInstance"); }
	if (PropertyFlags & EPropertyFlags::UObjectWrapper) { Buffer.push_back("UObjectWrapper"); }
	if (PropertyFlags & EPropertyFlags::HasGetValueTypeHash) { Buffer.push_back("HasGetValueTypeHash"); }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierPublic) { Buffer.push_back("NativeAccessSpecifierPublic"); }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierProtected) { Buffer.push_back("NativeAccessSpecifierProtected"); }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierPrivate) { Buffer.push_back("NativeAccessSpecifierPrivate"); }

	switch (Buffer.size())
	{
	case 0:
		return std::string("None");
		break;
	case 1:
		return std::string(Buffer[0]);
		break;
	default:
		std::ostringstream os;
		std::copy(Buffer.begin(), Buffer.end() - 1, std::ostream_iterator<const char*>(os, ", "));
		os << *Buffer.rbegin();
		return os.str();
	}
}

static std::string StringifyObjectFlags(EObjectFlags ObjFlags)
{
	std::vector<const char*> Buffer;

	if (ObjFlags & EObjectFlags::Public) { Buffer.push_back("Public"); }
	if (ObjFlags & EObjectFlags::Standalone) { Buffer.push_back("Standalone"); }
	if (ObjFlags & EObjectFlags::MarkAsNative) { Buffer.push_back("MarkAsNative"); }
	if (ObjFlags & EObjectFlags::Transactional) { Buffer.push_back("Transactional"); }
	if (ObjFlags & EObjectFlags::ClassDefaultObject) { Buffer.push_back("ClassDefaultObject"); }
	if (ObjFlags & EObjectFlags::ArchetypeObject) { Buffer.push_back("ArchetypeObject"); }
	if (ObjFlags & EObjectFlags::Transient) { Buffer.push_back("Transient"); }
	if (ObjFlags & EObjectFlags::MarkAsRootSet) { Buffer.push_back("MarkAsRootSet"); }
	if (ObjFlags & EObjectFlags::TagGarbageTemp) { Buffer.push_back("TagGarbageTemp"); }
	if (ObjFlags & EObjectFlags::NeedInitialization) { Buffer.push_back("NeedInitialization"); }
	if (ObjFlags & EObjectFlags::NeedLoad) { Buffer.push_back("NeedLoad"); }
	if (ObjFlags & EObjectFlags::KeepForCooker) { Buffer.push_back("KeepForCooker"); }
	if (ObjFlags & EObjectFlags::NeedPostLoad) { Buffer.push_back("NeedPostLoad"); }
	if (ObjFlags & EObjectFlags::NeedPostLoadSubobjects) { Buffer.push_back("NeedPostLoadSubobjects"); }
	if (ObjFlags & EObjectFlags::NewerVersionExists) { Buffer.push_back("NewerVersionExists"); }
	if (ObjFlags & EObjectFlags::BeginDestroyed) { Buffer.push_back("BeginDestroyed"); }
	if (ObjFlags & EObjectFlags::FinishDestroyed) { Buffer.push_back("FinishDestroyed"); }
	if (ObjFlags & EObjectFlags::BeingRegenerated) { Buffer.push_back("BeingRegenerated"); }
	if (ObjFlags & EObjectFlags::DefaultSubObject) { Buffer.push_back("DefaultSubObject"); }
	if (ObjFlags & EObjectFlags::WasLoaded) { Buffer.push_back("WasLoaded"); }
	if (ObjFlags & EObjectFlags::TextExportTransient) { Buffer.push_back("TextExportTransient"); }
	if (ObjFlags & EObjectFlags::LoadCompleted) { Buffer.push_back("LoadCompleted"); }
	if (ObjFlags & EObjectFlags::InheritableComponentTemplate) { Buffer.push_back("InheritableComponentTemplate"); }
	if (ObjFlags & EObjectFlags::DuplicateTransient) { Buffer.push_back("DuplicateTransient"); }
	if (ObjFlags & EObjectFlags::StrongRefOnFrame) { Buffer.push_back("StrongRefOnFrame"); }
	if (ObjFlags & EObjectFlags::NonPIEDuplicateTransient) { Buffer.push_back("NonPIEDuplicateTransient"); }
	if (ObjFlags & EObjectFlags::Dynamic) { Buffer.push_back("Dynamic"); }
	if (ObjFlags & EObjectFlags::WillBeLoaded) { Buffer.push_back("WillBeLoaded"); }

	switch (Buffer.size())
	{
	case 0:
		return std::string("None");
		break;
	case 1:
		return std::string(Buffer[0]);
		break;
	default:
		std::ostringstream os;
		std::copy(Buffer.begin(), Buffer.end() - 1, std::ostream_iterator<const char*>(os, ", "));
		os << *Buffer.rbegin();
		return os.str();
	}
}

static std::string StringifyClassCastFlags(EClassCastFlags CastFlags)
{
	std::vector<const char*> Buffer;

	if (CastFlags & EClassCastFlags::Field) { Buffer.push_back("Field"); }
	if (CastFlags & EClassCastFlags::Int8Property) { Buffer.push_back("Int8Property"); }
	if (CastFlags & EClassCastFlags::Enum) { Buffer.push_back("Enum"); }
	if (CastFlags & EClassCastFlags::Struct) { Buffer.push_back("Struct"); }
	if (CastFlags & EClassCastFlags::ScriptStruct) { Buffer.push_back("ScriptStruct"); }
	if (CastFlags & EClassCastFlags::Class) { Buffer.push_back("Class"); }
	if (CastFlags & EClassCastFlags::ByteProperty) { Buffer.push_back("ByteProperty"); }
	if (CastFlags & EClassCastFlags::IntProperty) { Buffer.push_back("IntProperty"); }
	if (CastFlags & EClassCastFlags::FloatProperty) { Buffer.push_back("FloatProperty"); }
	if (CastFlags & EClassCastFlags::UInt64Property) { Buffer.push_back("UInt64Property"); }
	if (CastFlags & EClassCastFlags::ClassProperty) { Buffer.push_back("ClassProperty"); }
	if (CastFlags & EClassCastFlags::UInt32Property) { Buffer.push_back("UInt32Property"); }
	if (CastFlags & EClassCastFlags::InterfaceProperty) { Buffer.push_back("InterfaceProperty"); }
	if (CastFlags & EClassCastFlags::NameProperty) { Buffer.push_back("NameProperty"); }
	if (CastFlags & EClassCastFlags::StrProperty) { Buffer.push_back("StrProperty"); }
	if (CastFlags & EClassCastFlags::Property) { Buffer.push_back("Property"); }
	if (CastFlags & EClassCastFlags::ObjectProperty) { Buffer.push_back("ObjectProperty"); }
	if (CastFlags & EClassCastFlags::BoolProperty) { Buffer.push_back("BoolProperty"); }
	if (CastFlags & EClassCastFlags::UInt16Property) { Buffer.push_back("UInt16Property"); }
	if (CastFlags & EClassCastFlags::Function) { Buffer.push_back("Function"); }
	if (CastFlags & EClassCastFlags::StructProperty) { Buffer.push_back("StructProperty"); }
	if (CastFlags & EClassCastFlags::ArrayProperty) { Buffer.push_back("ArrayProperty"); }
	if (CastFlags & EClassCastFlags::Int64Property) { Buffer.push_back("Int64Property"); }
	if (CastFlags & EClassCastFlags::DelegateProperty) { Buffer.push_back("DelegateProperty"); }
	if (CastFlags & EClassCastFlags::NumericProperty) { Buffer.push_back("NumericProperty"); }
	if (CastFlags & EClassCastFlags::MulticastDelegateProperty) { Buffer.push_back("MulticastDelegateProperty"); }
	if (CastFlags & EClassCastFlags::ObjectPropertyBase) { Buffer.push_back("ObjectPropertyBase"); }
	if (CastFlags & EClassCastFlags::WeakObjectProperty) { Buffer.push_back("WeakObjectProperty"); }
	if (CastFlags & EClassCastFlags::LazyObjectProperty) { Buffer.push_back("LazyObjectProperty"); }
	if (CastFlags & EClassCastFlags::SoftObjectProperty) { Buffer.push_back("SoftObjectProperty"); }
	if (CastFlags & EClassCastFlags::TextProperty) { Buffer.push_back("TextProperty"); }
	if (CastFlags & EClassCastFlags::Int16Property) { Buffer.push_back("Int16Property"); }
	if (CastFlags & EClassCastFlags::DoubleProperty) { Buffer.push_back("DoubleProperty"); }
	if (CastFlags & EClassCastFlags::SoftClassProperty) { Buffer.push_back("SoftClassProperty"); }
	if (CastFlags & EClassCastFlags::Package) { Buffer.push_back("Package"); }
	if (CastFlags & EClassCastFlags::Level) { Buffer.push_back("Level"); }
	if (CastFlags & EClassCastFlags::Actor) { Buffer.push_back("Actor"); }
	if (CastFlags & EClassCastFlags::PlayerController) { Buffer.push_back("PlayerController"); }
	if (CastFlags & EClassCastFlags::Pawn) { Buffer.push_back("Pawn"); }
	if (CastFlags & EClassCastFlags::SceneComponent) { Buffer.push_back("SceneComponent"); }
	if (CastFlags & EClassCastFlags::PrimitiveComponent) { Buffer.push_back("PrimitiveComponent"); }
	if (CastFlags & EClassCastFlags::SkinnedMeshComponent) { Buffer.push_back("SkinnedMeshComponent"); }
	if (CastFlags & EClassCastFlags::SkeletalMeshComponent) { Buffer.push_back("SkeletalMeshComponent"); }
	if (CastFlags & EClassCastFlags::Blueprint) { Buffer.push_back("Blueprint"); }
	if (CastFlags & EClassCastFlags::DelegateFunction) { Buffer.push_back("DelegateFunction"); }
	if (CastFlags & EClassCastFlags::StaticMeshComponent) { Buffer.push_back("StaticMeshComponent"); }
	if (CastFlags & EClassCastFlags::MapProperty) { Buffer.push_back("MapProperty"); }
	if (CastFlags & EClassCastFlags::SetProperty) { Buffer.push_back("SetProperty"); }
	if (CastFlags & EClassCastFlags::EnumProperty) { Buffer.push_back("EnumProperty"); }
	if (CastFlags & EClassCastFlags::SparseDelegateFunction) { Buffer.push_back("SparseDelegateFunction"); }
	if (CastFlags & EClassCastFlags::MulticastInlineDelegateProperty) { Buffer.push_back("MulticastInlineDelegateProperty"); }
	if (CastFlags & EClassCastFlags::MulticastSparseDelegateProperty) { Buffer.push_back("MulticastSparseDelegateProperty"); }
	if (CastFlags & EClassCastFlags::FieldPathProperty) { Buffer.push_back("MarkAsFieldPathPropertyRootSet"); }
	if (CastFlags & EClassCastFlags::ObjectPtrProperty) { Buffer.push_back("ObjectPtrProperty"); }
	if (CastFlags & EClassCastFlags::ClassPtrProperty) { Buffer.push_back("ClassPtrProperty"); }
	if (CastFlags & EClassCastFlags::LargeWorldCoordinatesRealProperty) { Buffer.push_back("LargeWorldCoordinatesRealProperty"); }

	switch (Buffer.size())
	{
	case 0:
		return std::string("None");
		break;
	case 1:
		return std::string(Buffer[0]);
		break;
	default:
		std::ostringstream os;
		std::copy(Buffer.begin(), Buffer.end() - 1, std::ostream_iterator<const char*>(os, ", "));
		os << *Buffer.rbegin();
		return os.str();
	}
}

static EMappingsTypeFlags EPropertyFlagsToMappingFlags(EClassCastFlags Flags)
{
	
	if (Flags & EClassCastFlags::ObjectProperty 
	|| Flags & EClassCastFlags::ClassProperty 
	|| Flags & EClassCastFlags::ObjectPtrProperty
	|| Flags & EClassCastFlags::ClassPtrProperty)
	{
		return EMappingsTypeFlags::ObjectProperty;
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		return EMappingsTypeFlags::StructProperty;
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
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return EMappingsTypeFlags::ArrayProperty;
	}
	else if (Flags & EClassCastFlags::FloatProperty)
	{
		return EMappingsTypeFlags::FloatProperty;
	}
	else if (Flags & EClassCastFlags::DoubleProperty)
	{
		return EMappingsTypeFlags::DoubleProperty;
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return EMappingsTypeFlags::BoolProperty;
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return EMappingsTypeFlags::StrProperty;
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return EMappingsTypeFlags::NameProperty;
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return EMappingsTypeFlags::TextProperty;
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		return EMappingsTypeFlags::EnumProperty;
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		return EMappingsTypeFlags::InterfaceProperty;
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		return EMappingsTypeFlags::MapProperty;
	}
	else if (Flags & EClassCastFlags::ByteProperty)
	{
		return EMappingsTypeFlags::ByteProperty;
	}
	else if (Flags & EClassCastFlags::MulticastDelegateProperty
	|| Flags & EClassCastFlags::MulticastInlineDelegateProperty
	|| Flags & EClassCastFlags::MulticastSparseDelegateProperty)
	{
		return EMappingsTypeFlags::MulticastDelegateProperty;
	}
	else if (Flags & EClassCastFlags::DelegateProperty)
	{
		return EMappingsTypeFlags::DelegateProperty;
	}
	else if (Flags & EClassCastFlags::SoftObjectProperty
	|| Flags & EClassCastFlags::SoftClassProperty)
	{
		return EMappingsTypeFlags::SoftObjectProperty;
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		return EMappingsTypeFlags::WeakObjectProperty;
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		return EMappingsTypeFlags::LazyObjectProperty;
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return EMappingsTypeFlags::SetProperty;
	}
	else if (Flags & EClassCastFlags::FieldPathProperty)
	{
		return EMappingsTypeFlags::FieldPathProperty;
	}

	return EMappingsTypeFlags::Unknown;
}