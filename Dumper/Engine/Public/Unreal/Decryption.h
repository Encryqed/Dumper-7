#pragma once

#include <cstdint>

namespace Decryption
{
	// Pointer decryptors (encrypted pointer value -> decrypted address)
	inline uint8_t* (*FFieldClass_SuperClass)(void* Ptr)                = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*FField_Owner)(void* Ptr)                          = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*FField_Class)(void* Ptr)                          = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*FField_Next)(void* Ptr)                           = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UObject_Vft)(void* Ptr)                           = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UObject_Class)(void* Ptr)                         = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UObject_Outer)(void* Ptr)                         = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UField_Next)(void* Ptr)                           = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UStruct_SuperStruct)(void* Ptr)                   = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UStruct_Children)(void* Ptr)                      = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UStruct_ChildProperties)(void* Ptr)               = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UClass_ClassDefaultObject)(void* Ptr)             = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*UFunction_ExecFunction)(void* Ptr)                = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*ByteProperty_Enum)(void* Ptr)                     = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*ObjectProperty_PropertyClass)(void* Ptr)          = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*ClassProperty_MetaClass)(void* Ptr)               = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*StructProperty_Struct)(void* Ptr)                 = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*ArrayProperty_Inner)(void* Ptr)                   = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*DelegateProperty_SignatureFunction)(void* Ptr)    = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*MapProperty_KeyProperty)(void* Ptr)               = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*MapProperty_ValueProperty)(void* Ptr)             = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*SetProperty_ElementProp)(void* Ptr)               = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*EnumProperty_UnderlayingProperty)(void* Ptr)      = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*EnumProperty_Enum)(void* Ptr)                     = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*FieldPathProperty_FieldClass)(void* Ptr)          = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*OptionalProperty_ValueProperty)(void* Ptr)        = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };
	inline uint8_t* (*OffsetFinder_PropertySizePointer)(void* Ptr)      = [](void* Ptr) -> uint8_t* { return static_cast<uint8_t*>(Ptr); };

	// Value decryptors (encrypted -> decrypted value)
	inline int32_t (*UObject_Flags)(int32_t Raw)    = [](int32_t Raw) -> int32_t { return Raw; };
	inline int32_t (*FField_Flags)(int32_t Raw)     = [](int32_t Raw) -> int32_t { return Raw; };
	inline int32_t (*FName_CompIdx)(int32_t Raw)    = [](int32_t Raw) -> int32_t { return Raw; };
	inline int32_t (*FName_Number)(int32_t Raw)     = [](int32_t Raw) -> int32_t { return Raw; };

	void Init();
}

/*
 * Example:
 *   DECRYPT_UObject_Class([](void* Ptr) -> uint8_t* {
 *       return reinterpret_cast<uint8_t*>(uintptr_t(Ptr) ^ 0x1234ULL);
 *   })
 */
#define DECRYPT_FFieldClass_SuperClass(L)                (Decryption::FFieldClass_SuperClass = (L));
#define DECRYPT_FField_Owner(L)                          (Decryption::FField_Owner = (L));
#define DECRYPT_FField_Class(L)                          (Decryption::FField_Class = (L));
#define DECRYPT_FField_Next(L)                           (Decryption::FField_Next = (L));
#define DECRYPT_UObject_Vft(L)                           (Decryption::UObject_Vft = (L));
#define DECRYPT_UObject_Class(L)                         (Decryption::UObject_Class = (L));
#define DECRYPT_UObject_Outer(L)                         (Decryption::UObject_Outer = (L));
#define DECRYPT_UField_Next(L)                           (Decryption::UField_Next = (L));
#define DECRYPT_UStruct_SuperStruct(L)                   (Decryption::UStruct_SuperStruct = (L));
#define DECRYPT_UStruct_Children(L)                      (Decryption::UStruct_Children = (L));
#define DECRYPT_UStruct_ChildProperties(L)               (Decryption::UStruct_ChildProperties = (L));
#define DECRYPT_UClass_ClassDefaultObject(L)             (Decryption::UClass_ClassDefaultObject = (L));
#define DECRYPT_UFunction_ExecFunction(L)                (Decryption::UFunction_ExecFunction = (L));
#define DECRYPT_ByteProperty_Enum(L)                     (Decryption::ByteProperty_Enum = (L));
#define DECRYPT_ObjectProperty_PropertyClass(L)          (Decryption::ObjectProperty_PropertyClass = (L));
#define DECRYPT_ClassProperty_MetaClass(L)               (Decryption::ClassProperty_MetaClass = (L));
#define DECRYPT_StructProperty_Struct(L)                 (Decryption::StructProperty_Struct = (L));
#define DECRYPT_ArrayProperty_Inner(L)                   (Decryption::ArrayProperty_Inner = (L));
#define DECRYPT_DelegateProperty_SignatureFunction(L)    (Decryption::DelegateProperty_SignatureFunction = (L));
#define DECRYPT_MapProperty_KeyProperty(L)               (Decryption::MapProperty_KeyProperty = (L));
#define DECRYPT_MapProperty_ValueProperty(L)             (Decryption::MapProperty_ValueProperty = (L));
#define DECRYPT_SetProperty_ElementProp(L)               (Decryption::SetProperty_ElementProp = (L));
#define DECRYPT_EnumProperty_UnderlayingProperty(L)      (Decryption::EnumProperty_UnderlayingProperty = (L));
#define DECRYPT_EnumProperty_Enum(L)                     (Decryption::EnumProperty_Enum = (L));
#define DECRYPT_FieldPathProperty_FieldClass(L)          (Decryption::FieldPathProperty_FieldClass = (L));
#define DECRYPT_OptionalProperty_ValueProperty(L)        (Decryption::OptionalProperty_ValueProperty = (L));
#define DECRYPT_OffsetFinder_PropertySizePointer(L)      (Decryption::OffsetFinder_PropertySizePointer = (L));
#define DECRYPT_UObject_Flags(L)                         (Decryption::UObject_Flags = (L));
#define DECRYPT_FField_Flags(L)                          (Decryption::FField_Flags = (L));
#define DECRYPT_FName_CompIdx(L)                         (Decryption::FName_CompIdx = (L));
#define DECRYPT_FName_Number(L)                          (Decryption::FName_Number = (L));
