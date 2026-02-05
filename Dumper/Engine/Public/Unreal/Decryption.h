#pragma once

#include <cstdint>

namespace Decryption
{
	// Pointer decryptors: take address of the member (where the pointer is stored); read + decrypt inside, return decrypted address.
	inline uint8_t* (*FFieldClass_SuperClass)(const void* MemberAddress)                = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*FField_Owner)(const void* MemberAddress)                          = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*FField_Class)(const void* MemberAddress)                          = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*FField_Next)(const void* MemberAddress)                           = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UObject_Vft)(const void* MemberAddress)                           = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UObject_Class)(const void* MemberAddress)                         = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UObject_Outer)(const void* MemberAddress)                         = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UField_Next)(const void* MemberAddress)                           = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UStruct_SuperStruct)(const void* MemberAddress)                   = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UStruct_Children)(const void* MemberAddress)                      = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UStruct_ChildProperties)(const void* MemberAddress)               = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UClass_ClassDefaultObject)(const void* MemberAddress)             = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*UFunction_ExecFunction)(const void* MemberAddress)                = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*ByteProperty_Enum)(const void* MemberAddress)                     = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*ObjectProperty_PropertyClass)(const void* MemberAddress)          = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*ClassProperty_MetaClass)(const void* MemberAddress)               = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*StructProperty_Struct)(const void* MemberAddress)                 = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*ArrayProperty_Inner)(const void* MemberAddress)                   = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*DelegateProperty_SignatureFunction)(const void* MemberAddress)    = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*MapProperty_KeyProperty)(const void* MemberAddress)               = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*MapProperty_ValueProperty)(const void* MemberAddress)             = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*SetProperty_ElementProp)(const void* MemberAddress)               = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*EnumProperty_UnderlayingProperty)(const void* MemberAddress)      = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*EnumProperty_Enum)(const void* MemberAddress)                     = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*FieldPathProperty_FieldClass)(const void* MemberAddress)          = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*OptionalProperty_ValueProperty)(const void* MemberAddress)        = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };
	inline uint8_t* (*OffsetFinder_PropertySizePointer)(const void* MemberAddress)      = [](const void* MemberAddress) -> uint8_t* { return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress)); };

	// Value decryptors (encrypted -> decrypted value)
	inline int32_t (*UObject_Flags)(const void* Address)    = [](const void* Address) -> int32_t { return *reinterpret_cast<const int32_t*>(Address); };
	inline int32_t (*FField_Flags)(const void* Address)     = [](const void* Address) -> int32_t { return *reinterpret_cast<const int32_t*>(Address); };
	inline int32_t (*FName_CompIdx)(const void* Address)    = [](const void* Address) -> int32_t { return *reinterpret_cast<const int32_t*>(Address); };
	inline int32_t (*FName_Number)(const void* Address)     = [](const void* Address) -> int32_t { return *reinterpret_cast<const int32_t*>(Address); };

	void Init();
}

/*
 * Example pointer
 *   DECRYPT_UObject_Class([](const void* MemberAddress) -> uint8_t* {
 *       const uintptr_t Stored = *reinterpret_cast<const uintptr_t*>(MemberAddress);
 *       const uintptr_t Key = 0xDEADBEEFULL;
 *       void* const DecryptedPtr = reinterpret_cast<void*>(Stored ^ Key);
 *       return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(DecryptedPtr));
 *   })
 *
 * Example value (with key 4 bytes after the member):
 *   DECRYPT_FName_CompIdx([](const void* Address) -> int32_t {
 *       const int32_t Raw = *reinterpret_cast<const int32_t*>(Address);
 *       const int32_t Key = *reinterpret_cast<const int32_t*>(static_cast<const uint8_t*>(Address) + 0x4);
 *       return Raw ^ Key;
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
