#include "Unreal/Decryption.h"

void Decryption::Init()
{
/* --- UObject --- */
DECRYPT_UObject_Vft([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_UObject_Class([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_UObject_Outer([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_UObject_Flags([](int32_t Raw) -> int32_t {
	return Raw;
})

/* --- UField --- */
DECRYPT_UField_Next([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- UStruct --- */
DECRYPT_UStruct_SuperStruct([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_UStruct_Children([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_UStruct_ChildProperties([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- UClass --- */
DECRYPT_UClass_ClassDefaultObject([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- UFunction --- */
DECRYPT_UFunction_ExecFunction([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FFieldClass --- */
DECRYPT_FFieldClass_SuperClass([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FField --- */
DECRYPT_FField_Owner([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_FField_Class([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_FField_Next([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_FField_Flags([](int32_t Raw) -> int32_t {
	return Raw;
})

/* --- FName --- */
DECRYPT_FName_CompIdx([](int32_t Raw) -> int32_t {
	return Raw;
})

DECRYPT_FName_Number([](int32_t Raw) -> int32_t {
	return Raw;
})

/* --- FByteProperty --- */
DECRYPT_ByteProperty_Enum([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FObjectProperty --- */
DECRYPT_ObjectProperty_PropertyClass([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FClassProperty --- */
DECRYPT_ClassProperty_MetaClass([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FStructProperty --- */
DECRYPT_StructProperty_Struct([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FArrayProperty --- */
DECRYPT_ArrayProperty_Inner([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FDelegateProperty / FMulticastDelegateProperty --- */
DECRYPT_DelegateProperty_SignatureFunction([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FMapProperty --- */
DECRYPT_MapProperty_KeyProperty([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_MapProperty_ValueProperty([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FSetProperty --- */
DECRYPT_SetProperty_ElementProp([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FEnumProperty --- */
DECRYPT_EnumProperty_UnderlayingProperty([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

DECRYPT_EnumProperty_Enum([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FFieldPathProperty --- */
DECRYPT_FieldPathProperty_FieldClass([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- FOptionalProperty --- */
DECRYPT_OptionalProperty_ValueProperty([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})

/* --- OffsetFinder (property+PropertySize pointer reads) --- */
DECRYPT_OffsetFinder_PropertySizePointer([](void* Ptr) -> uint8_t* {
	return static_cast<uint8_t*>(Ptr);
})
}
