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
																																																																																		

enum class EPropertyFlags : uint64_t
{
	None = 0,

	Edit = 0x0000000000000001,	// Property is user-settable in the editor.
	ConstParm = 0x0000000000000002,	// This is a constant function parameter
	BlueprintVisible = 0x0000000000000004,	// This property can be read by blueprint code
	ExportObject = 0x0000000000000008,	// Object can be exported with actor.
	BlueprintReadOnly = 0x0000000000000010,	// This property cannot be modified by blueprint code
	Net = 0x0000000000000020,	// Property is relevant to network replication.
	EditFixedSize = 0x0000000000000040,	// Indicates that elements of an array can be modified, but its size cannot be changed.
	Parm = 0x0000000000000080,	// Function/When call parameter.
	OutParm = 0x0000000000000100,	// Value is copied out after function call.
	ZeroConstructor = 0x0000000000000200,	// memset is fine for construction
	ReturnParm = 0x0000000000000400,	// Return value.
	DisableEditOnTemplate = 0x0000000000000800,	// Disable editing of this property on an archetype/sub-blueprint
	//CPF_      						= 0x0000000000001000,	// 
	Transient = 0x0000000000002000,	// Property is transient: shouldn't be saved or loaded, except for Blueprint CDOs.
	Config = 0x0000000000004000,	// Property should be loaded/saved as permanent profile.
	//CPF_								= 0x0000000000008000,	// 
	DisableEditOnInstance = 0x0000000000010000,	// Disable editing on an instance of this class
	EditConst = 0x0000000000020000,	// Property is uneditable in the editor.
	GlobalConfig = 0x0000000000040000,	// Load config from base class, not subclass.
	InstancedReference = 0x0000000000080000,	// Property is a component references.
	//CPF_								= 0x0000000000100000,
	DuplicateTransient = 0x0000000000200000,	// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
	SubobjectReference = 0x0000000000400000,	// Property contains subobject references (TSubobjectPtr)
	//CPF_    							= 0x0000000000800000,	// 
	SaveGame = 0x0000000001000000,	// Property should be serialized for save games
	NoClear = 0x0000000002000000,	// Hide clear (and browse) button.
	//CPF_  							= 0x0000000004000000,	//
	ReferenceParm = 0x0000000008000000,	// Value is passed by reference; OutParam and Param should also be set.
	BlueprintAssignable = 0x0000000010000000,	// MC Delegates only.  Property should be exposed for assigning in blueprint code
	Deprecated = 0x0000000020000000,	// Property is deprecated.  Read it from an archive, but don't save it.
	IsPlainOldData = 0x0000000040000000,	// If this is set, then the property can be memcopied instead of CopyCompleteValue / CopySingleValue
	RepSkip = 0x0000000080000000,	// Not replicated. For non replicated properties in replicated structs 
	RepNotify = 0x0000000100000000,	// Notify actors when a property is replicated
	Interp = 0x0000000200000000,	// interpolatable property for use with matinee
	NonTransactional = 0x0000000400000000,	// Property isn't transacted
	EditorOnly = 0x0000000800000000,	// Property should only be loaded in the editor
	NoDestructor = 0x0000001000000000,	// No destructor
	//CPF_								= 0x0000002000000000,	//
	AutoWeak = 0x0000004000000000,	// Only used for weak pointers, means the export type is autoweak
	ContainsInstancedReference = 0x0000008000000000,	// Property contains component references.
	AssetRegistrySearchable = 0x0000010000000000,	// asset instances will add properties with this flag to the asset registry automatically
	SimpleDisplay = 0x0000020000000000,	// The property is visible by default in the editor details view
	AdvancedDisplay = 0x0000040000000000,	// The property is advanced and not visible by default in the editor details view
	Protected = 0x0000080000000000,	// property is protected from the perspective of script
	BlueprintCallable = 0x0000100000000000,	// MC Delegates only.  Property should be exposed for calling in blueprint code
	BlueprintAuthorityOnly = 0x0000200000000000,	// MC Delegates only.  This delegate accepts (only in blueprint) only events with BlueprintAuthorityOnly.
	TextExportTransient = 0x0000400000000000,	// Property shouldn't be exported to text format (e.g. copy/paste)
	NonPIEDuplicateTransient = 0x0000800000000000,	// Property should only be copied in PIE
	ExposeOnSpawn = 0x0001000000000000,	// Property is exposed on spawn
	PersistentInstance = 0x0002000000000000,	// A object referenced by the property is duplicated like a component. (Each actor should have an own instance.)
	UObjectWrapper = 0x0004000000000000,	// Property was parsed as a wrapper class like TSubclassOf<T>, FScriptInterface etc., rather than a USomething*
	HasGetValueTypeHash = 0x0008000000000000,	// This property can generate a meaningful hash value.
	NativeAccessSpecifierPublic = 0x0010000000000000,	// Public native access specifier
	NativeAccessSpecifierProtected = 0x0020000000000000,	// Protected native access specifier
	NativeAccessSpecifierPrivate = 0x0040000000000000,	// Private native access specifier
	SkipSerialization = 0x0080000000000000,	// Property shouldn't be serialized, can still be exported to text
};

enum class EFunctionFlags : uint32_t
{
	None = 0x00000000,

	Final = 0x00000001,
	RequiredAPI = 0x00000002,
	BlueprintAuthorityOnly = 0x00000004,
	BlueprintCosmetic = 0x00000008,
	Net = 0x00000040,
	NetReliable = 0x00000080,
	NetRequest = 0x00000100,
	Exec = 0x00000200,
	Native = 0x00000400,
	Event = 0x00000800,
	NetResponse = 0x00001000,
	Static = 0x00002000,
	NetMulticast = 0x00004000,
	UbergraphFunction = 0x00008000,
	MulticastDelegate = 0x00010000,
	Public = 0x00020000,
	Private = 0x00040000,
	Protected = 0x00080000,
	Delegate = 0x00100000,
	NetServer = 0x00200000,
	HasOutParms = 0x00400000,
	HasDefaults = 0x00800000,
	NetClient = 0x01000000,
	DLLImport = 0x02000000,
	BlueprintCallable = 0x04000000,
	BlueprintEvent = 0x08000000,
	BlueprintPure = 0x10000000,
	EditorOnly = 0x20000000,
	Const = 0x40000000,
	NetValidate = 0x80000000,

	AllFlags = 0xFFFFFFFF,
};

enum class EObjectFlags
{
	// Do not add new flags unless they truly belong here. There are alternatives.
	// if you change any the bit of any of the RF_Load flags, then you will need legacy serialization
	RF_NoFlags = 0x00000000,	///< No flags, used to avoid a cast

	// This first group of flags mostly has to do with what kind of object it is. Other than transient, these are the persistent object flags.
	// The garbage collector also tends to look at these.
	RF_Public = 0x00000001,	///< Object is visible outside its package.
	RF_Standalone = 0x00000002,	///< Keep object around for editing even if unreferenced.
	RF_MarkAsNative = 0x00000004,	///< Object (UField) will be marked as native on construction (DO NOT USE THIS FLAG in HasAnyFlags() etc)
	RF_Transactional = 0x00000008,	///< Object is transactional.
	RF_ClassDefaultObject = 0x00000010,	///< This object is its class's default object
	RF_ArchetypeObject = 0x00000020,	///< This object is a template for another object - treat like a class default object
	RF_Transient = 0x00000040,	///< Don't save object.

	// This group of flags is primarily concerned with garbage collection.
	RF_MarkAsRootSet = 0x00000080,	///< Object will be marked as root set on construction and not be garbage collected, even if unreferenced (DO NOT USE THIS FLAG in HasAnyFlags() etc)
	RF_TagGarbageTemp = 0x00000100,	///< This is a temp user flag for various utilities that need to use the garbage collector. The garbage collector itself does not interpret it.

	// The group of flags tracks the stages of the lifetime of a uobject
	RF_NeedInitialization = 0x00000200,	///< This object has not completed its initialization process. Cleared when ~FObjectInitializer completes
	RF_NeedLoad = 0x00000400,	///< During load, indicates object needs loading.
	RF_KeepForCooker = 0x00000800,	///< Keep this object during garbage collection because it's still being used by the cooker
	RF_NeedPostLoad = 0x00001000,	///< Object needs to be postloaded.
	RF_NeedPostLoadSubobjects = 0x00002000,	///< During load, indicates that the object still needs to instance subobjects and fixup serialized component references
	RF_NewerVersionExists = 0x00004000,	///< Object has been consigned to oblivion due to its owner package being reloaded, and a newer version currently exists
	RF_BeginDestroyed = 0x00008000,	///< BeginDestroy has been called on the object.
	RF_FinishDestroyed = 0x00010000,	///< FinishDestroy has been called on the object.

	// Misc. Flags
	RF_BeingRegenerated = 0x00020000,	///< Flagged on UObjects that are used to create UClasses (e.g. Blueprints) while they are regenerating their UClass on load (See FLinkerLoad::CreateExport())
	RF_DefaultSubObject = 0x00040000,	///< Flagged on subobjects that are defaults
	RF_WasLoaded = 0x00080000,	///< Flagged on UObjects that were loaded
	RF_TextExportTransient = 0x00100000,	///< Do not export object to text form (e.g. copy/paste). Generally used for sub-objects that can be regenerated from data in their parent object.
	RF_LoadCompleted = 0x00200000,	///< Object has been completely serialized by linkerload at least once. DO NOT USE THIS FLAG, It should be replaced with RF_WasLoaded.
	RF_InheritableComponentTemplate = 0x00400000, ///< Archetype of the object can be in its super class
	RF_DuplicateTransient = 0x00800000, ///< Object should not be included in any type of duplication (copy/paste, binary duplication, etc.)
	RF_StrongRefOnFrame = 0x01000000,	///< References to this object from persistent function frame are handled as strong ones.
	RF_NonPIEDuplicateTransient = 0x02000000,  ///< Object should not be included for duplication unless it's being duplicated for a PIE session
	RF_Dynamic = 0x04000000, // Field Only. Dynamic field - doesn't get constructed during static initialization, can be constructed multiple times
	RF_WillBeLoaded = 0x08000000, // This object was constructed during load and will be loaded shortly
};

enum class EClassCastFlags : uint64_t
{
	None = 0x0000000000000000,

	UField							= 0x0000000000000001,
	UInt8Property					= 0x0000000000000002,
	UEnum							= 0x0000000000000004,
	UStruct							= 0x0000000000000008,
	UScriptStruct					= 0x0000000000000010,
	UClass							= 0x0000000000000020,
	UByteProperty					= 0x0000000000000040,
	UIntProperty					= 0x0000000000000080,
	UFloatProperty					= 0x0000000000000100,
	UUInt64Property					= 0x0000000000000200,
	UClassProperty					= 0x0000000000000400,
	UUInt32Property					= 0x0000000000000800,
	UInterfaceProperty				= 0x0000000000001000,
	UNameProperty					= 0x0000000000002000,
	UStrProperty					= 0x0000000000004000,
	UProperty						= 0x0000000000008000,
	UObjectProperty					= 0x0000000000010000,
	UBoolProperty					= 0x0000000000020000,
	UUInt16Property					= 0x0000000000040000,
	UFunction						= 0x0000000000080000,
	UStructProperty					= 0x0000000000100000,
	UArrayProperty					= 0x0000000000200000,
	UInt64Property					= 0x0000000000400000,
	UDelegateProperty				= 0x0000000000800000,
	UNumericProperty				= 0x0000000001000000,
	UMulticastDelegateProperty		= 0x0000000002000000,
	UObjectPropertyBase				= 0x0000000004000000,
	UWeakObjectProperty				= 0x0000000008000000,
	ULazyObjectProperty				= 0x0000000010000000,
	USoftObjectProperty				= 0x0000000020000000,
	UTextProperty					= 0x0000000040000000,
	UInt16Property					= 0x0000000080000000,
	UDoubleProperty					= 0x0000000100000000,
	USoftClassProperty				= 0x0000000200000000,
	UPackage						= 0x0000000400000000,
	ULevel							= 0x0000000800000000,
	AActor							= 0x0000001000000000,
	APlayerController				= 0x0000002000000000,
	APawn							= 0x0000004000000000,
	USceneComponent					= 0x0000008000000000,
	UPrimitiveComponent				= 0x0000010000000000,
	USkinnedMeshComponent			= 0x0000020000000000,
	USkeletalMeshComponent			= 0x0000040000000000,
	UBlueprint						= 0x0000080000000000,
	UDelegateFunction				= 0x0000100000000000,
	UStaticMeshComponent			= 0x0000200000000000,
	UMapProperty					= 0x0000400000000000,
	USetProperty					= 0x0000800000000000,
	UEnumProperty					= 0x0001000000000000,
};

ENUM_OPERATORS(EObjectFlags);
ENUM_OPERATORS(EFunctionFlags);
ENUM_OPERATORS(EPropertyFlags);
ENUM_OPERATORS(EClassCastFlags);

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
		return std::string("");
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
		return std::string("");
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
