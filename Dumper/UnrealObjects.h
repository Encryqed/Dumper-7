#pragma once

#include <vector>
#include <unordered_map>
#include "Enums.h"
#include "UnrealTypes.h"

class UEClass;
class UEFField;
class UEObject;
class UEProperty;

class UEFFieldClass
{
protected:
	uint8* Class;

public:

	UEFFieldClass() = default;

	UEFFieldClass(void* NewFieldClass)
		: Class(reinterpret_cast<uint8*>(NewFieldClass))
	{
	}

	UEFFieldClass(const UEFFieldClass& OldFieldClass)
		: Class(reinterpret_cast<uint8*>(OldFieldClass.Class))
	{
	}

	void* GetAddress();

	EFieldClassID GetId() const;

	EClassCastFlags GetCastFlags() const;
	EClassFlags GetClassFlags() const;
	UEFFieldClass GetSuper() const;
	FName GetFName() const;

	bool IsType(EClassCastFlags Flags) const;

	std::string GetName() const;
	std::string GetValidName() const;
	std::string GetCppName() const;
};

class UEFField
{
protected:
	uint8* Field;

public:

	UEFField() = default;

	UEFField(void* NewField)
		: Field(reinterpret_cast<uint8*>(NewField))
	{
	}

	UEFField(const UEFField& OldField)
		: Field(reinterpret_cast<uint8*>(OldField.Field))
	{
	}

	void* GetAddress();

	EObjectFlags GetFlags() const;
	class UEObject GetOwnerAsUObject() const;
	class UEFField GetOwnerAsFField() const;
	class UEObject GetOwnerUObject() const;
	class UEObject GetOutermost() const;
	UEFFieldClass GetClass() const;
	FName GetFName() const;
	UEFField GetNext() const;

	template<typename UEType>
	UEType Cast() const;

	bool IsOwnerUObject() const;
	bool IsA(EClassCastFlags Flags) const;

	std::string GetName() const;
	std::string GetValidName() const;
	std::string GetCppName() const;

	explicit operator bool() const;
	bool operator==(const UEFField& Other) const;
	bool operator!=(const UEFField& Other) const;
};

class UEObject
{
private:
	static void(*PE)(void*, void*, void*);

protected:
	uint8* Object;

public:

	UEObject() = default;

	UEObject(void* NewObject)
		: Object(reinterpret_cast<uint8*>(NewObject))
	{
	}

	UEObject(const UEObject& OldObject)
		: Object(reinterpret_cast<uint8*>(OldObject.Object))
	{
	}

	void* GetAddress();

	EObjectFlags GetFlags() const;
	int32 GetIndex() const;
	UEClass GetClass() const;
	FName GetFName() const;
	UEObject GetOuter() const;

	bool HasAnyFlags(EObjectFlags Flags) const;

	template<typename UEType>
	UEType Cast();

	template<typename UEType>
	const UEType Cast() const;

	bool IsA(EClassCastFlags TypeFlags) const;

	UEObject GetOutermost() const;

	std::string GetName() const;
	std::string GetValidName() const;
	std::string GetCppName() const;
	std::string GetFullName() const;

	explicit operator bool() const;
	explicit operator uint8*();
	bool operator==(const UEObject& Other) const;
	bool operator!=(const UEObject& Other) const;

	void ProcessEvent(class UEFunction Func, void* Params);
};

class UEField : public UEObject
{
	using UEObject::UEObject;

public:
	UEField GetNext() const;
	bool IsNextValid() const;
};

class UEEnum : public UEField
{
	using UEField::UEField;

public:
	static std::unordered_map<int32, std::string> BigEnums; //ObjectArray::GetAllPackages()

	std::vector<TPair<FName, int64>> GetNameValuePairs() const;
	std::string GetSingleName(int32 Index) const;
	std::string GetEnumTypeAsStr() const;
};

class UEStruct : public UEField
{
	using UEField::UEField;

public:
	static std::unordered_map<int32 /*StructIdx*/, uint32 /*RealSize*/> StructSizes;

public:
	UEStruct GetSuper() const;
	UEField GetChild() const;
	UEFField GetChildProperties() const;
	int32 GetStructSize() const;

	std::vector<UEProperty> GetProperties() const;

	bool HasMembers() const;
};

class UEFunction : public UEStruct
{
	using UEStruct::UEStruct;

public:
	EFunctionFlags GetFunctionFlags() const;
	bool HasFlags(EFunctionFlags Flags) const;

	std::string StringifyFlags() const;
	std::string GetParamStructName() const;
};

class UEClass : public UEStruct
{
	using UEStruct::UEStruct;

public:
	EClassCastFlags GetCastFlags() const;
	bool IsType(EClassCastFlags TypeFlag) const;
	bool HasType(UEClass TypeClass) const;
	UEObject GetDefaultObject() const;

	UEFunction GetFunction(const std::string& ClassName, const std::string& FuncName) const;
};

class UEProperty
{
protected:
	uint8* Base;

public:
	static std::unordered_map<std::string /* Property Name */, uint32 /* Property Size */> UnknownProperties;

	UEProperty() = default;

	UEProperty(void* NewProperty)
		: Base(reinterpret_cast<uint8*>(NewProperty))
	{
	}

	UEProperty(const UEProperty& OldProperty)
		: Base(reinterpret_cast<uint8*>(OldProperty.Base))
	{
	}

public:
	void* GetAddress();

	std::pair<UEClass, UEFFieldClass> GetClass() const;

	template<typename UEType>
	UEType Cast();

	template<typename UEType>
	const UEType Cast() const;

	bool IsA(EClassCastFlags TypeFlags) const;

	bool IsTypeSupported() const;

	FName GetFName() const;
	int32 GetArrayDim() const;
	int32 GetSize() const;
	int32 GetOffset() const;
	EPropertyFlags GetPropertyFlags() const;
	EMappingsTypeFlags GetMappingType() const;
	bool HasPropertyFlags(EPropertyFlags PropertyFlag) const;

	UEObject GetOutermost() const;

	std::string GetName() const;
	std::string GetValidName() const;

	std::string GetCppType() const;

	std::string StringifyFlags() const;

public:
	static consteval EClassCastFlags GetSupportedProperties()
	{
		return EClassCastFlags::ByteProperty | EClassCastFlags::UInt16Property | EClassCastFlags::UInt32Property | EClassCastFlags::UInt64Property
			| EClassCastFlags::Int8Property | EClassCastFlags::Int16Property | EClassCastFlags::IntProperty | EClassCastFlags::Int64Property
			| EClassCastFlags::FloatProperty | EClassCastFlags::DoubleProperty | EClassCastFlags::ClassProperty | EClassCastFlags::NameProperty
			| EClassCastFlags::StrProperty | EClassCastFlags::TextProperty | EClassCastFlags::BoolProperty | EClassCastFlags::StructProperty
			| EClassCastFlags::ArrayProperty | EClassCastFlags::WeakObjectProperty | EClassCastFlags::LazyObjectProperty | EClassCastFlags::SoftClassProperty
			| EClassCastFlags::SoftObjectProperty | EClassCastFlags::ObjectProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty
			| EClassCastFlags::EnumProperty | EClassCastFlags::InterfaceProperty;
	}
};

class UEByteProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEEnum GetEnum() const;

	std::string GetCppType() const;
};

class UEBoolProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	uint8 GetFieldMask() const;
	uint8 GetBitIndex() const;
	bool IsNativeBool() const;

	std::string GetCppType() const;
};

class UEObjectProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEClass GetPropertyClass() const;

	std::string GetCppType() const;
};

class UEClassProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	UEClass GetMetaClass() const;

	std::string GetCppType() const;
};

class UEWeakObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType() const;
};

class UELazyObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType() const;
};

class UESoftObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType() const;
};

class UESoftClassProperty : public UEClassProperty
{
	using UEClassProperty::UEClassProperty;

public:
	std::string GetCppType() const;
};

class UEInterfaceProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType() const;
};

class UEStructProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEStruct GetUnderlayingStruct() const;

	std::string GetCppType() const;
};

class UEArrayProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetInnerProperty() const;

	std::string GetCppType() const;
};

class UEMapProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetKeyProperty() const;
	UEProperty GetValueProperty() const;

	std::string GetCppType() const;
};

class UESetProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetElementProperty() const;

	std::string GetCppType() const;
};

class UEEnumProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetUnderlayingProperty() const;
	UEEnum GetEnum() const;

	std::string GetCppType() const;
};
