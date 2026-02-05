#include "Unreal/Decryption.h"

void Decryption::Init()
{
/* --- UObject --- */
DECRYPT_UObject_Vft([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_UObject_Class([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_UObject_Outer([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_UObject_Flags([](const void* Address) -> int32_t {
	return *reinterpret_cast<const int32_t*>(Address);
})

/* --- UField --- */
DECRYPT_UField_Next([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- UStruct --- */
DECRYPT_UStruct_SuperStruct([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_UStruct_Children([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_UStruct_ChildProperties([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- UClass --- */
DECRYPT_UClass_ClassDefaultObject([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- UFunction --- */
DECRYPT_UFunction_ExecFunction([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FFieldClass --- */
DECRYPT_FFieldClass_SuperClass([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FField --- */
DECRYPT_FField_Owner([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_FField_Class([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_FField_Next([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_FField_Flags([](const void* Address) -> int32_t {
	return *reinterpret_cast<const int32_t*>(Address);
})

/* --- FName --- */
DECRYPT_FName_CompIdx([](const void* Address) -> int32_t {
	return *reinterpret_cast<const int32_t*>(Address);
})

DECRYPT_FName_Number([](const void* Address) -> int32_t {
	return *reinterpret_cast<const int32_t*>(Address);
})

/* --- FByteProperty --- */
DECRYPT_ByteProperty_Enum([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FObjectProperty --- */
DECRYPT_ObjectProperty_PropertyClass([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FClassProperty --- */
DECRYPT_ClassProperty_MetaClass([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FStructProperty --- */
DECRYPT_StructProperty_Struct([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FArrayProperty --- */
DECRYPT_ArrayProperty_Inner([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FDelegateProperty / FMulticastDelegateProperty --- */
DECRYPT_DelegateProperty_SignatureFunction([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FMapProperty --- */
DECRYPT_MapProperty_KeyProperty([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_MapProperty_ValueProperty([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FSetProperty --- */
DECRYPT_SetProperty_ElementProp([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FEnumProperty --- */
DECRYPT_EnumProperty_UnderlayingProperty([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

DECRYPT_EnumProperty_Enum([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FFieldPathProperty --- */
DECRYPT_FieldPathProperty_FieldClass([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- FOptionalProperty --- */
DECRYPT_OptionalProperty_ValueProperty([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})

/* --- OffsetFinder (property+PropertySize pointer reads) --- */
DECRYPT_OffsetFinder_PropertySizePointer([](const void* MemberAddress) -> uint8_t* {
	return static_cast<uint8_t*>(*reinterpret_cast<void* const*>(MemberAddress));
})
}
