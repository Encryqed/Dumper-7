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

	EFieldClassID GetId();

	EClassCastFlags GetCastFlags();
	EClassFlags GetClassFlags();
	UEFFieldClass GetSuper();
	FName GetFName();

	bool IsType(EClassCastFlags Flags);

	std::string GetName();
	std::string GetValidName();
	std::string GetCppName();
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

	EObjectFlags GetFlags();
	class UEObject GetOwnerAsUObject();
	class UEFField GetOwnerAsFField();
	class UEObject GetOwnerUObject();
	class UEObject GetOutermost();
	UEFFieldClass GetClass();
	FName GetFName();
	UEFField GetNext();

	template<typename UEType>
	UEType Cast() const;

	bool IsOwnerUObject();
	bool IsA(EClassCastFlags Flags);

	std::string GetName();
	std::string GetValidName();
	std::string GetCppName();

	explicit operator bool();
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

	EObjectFlags GetFlags();
	int32 GetIndex();
	UEClass GetClass();
	FName GetFName();
	UEObject GetOuter();

	bool HasAnyFlags(EObjectFlags Flags);

	template<typename UEType>
	UEType Cast() const;

	bool IsA(EClassCastFlags TypeFlags);

	UEObject GetOutermost();

	std::string GetName();
	std::string GetValidName();
	std::string GetCppName();
	std::string GetFullName();

	explicit operator bool();
	explicit operator uint8*();
	bool operator==(const UEObject& Other) const;
	bool operator!=(const UEObject& Other) const;

	void ProcessEvent(class UEFunction Func, void* Params);
};

class UEField : public UEObject
{
	using UEObject::UEObject;

public:
	UEField GetNext();
	bool IsNextValid();
};

class UEEnum : public UEField
{
	using UEField::UEField;

public:
	static std::unordered_map<int32, std::string> BigEnums; //ObjectArray::GetAllPackages()

	std::vector<TPair<FName, int64>> GetNameValuePairs();
	std::string GetSingleName(int32 Index);
	std::string GetEnumTypeAsStr();

	std::string UNSAFEGetCPPType();
	std::string UNSAFEGetDeclarationType();
};

class UEStruct : public UEField
{
	using UEField::UEField;

public:
	static std::unordered_map<int32 /*StructIdx*/, uint32 /*RealSize*/> StructSizes;

public:
	UEStruct GetSuper();
	UEField GetChild();
	UEFField GetChildProperties();
	int32 GetStructSize();

	std::vector<UEProperty> GetProperties();

	bool HasMembers();
};

class UEFunction : public UEStruct
{
	using UEStruct::UEStruct;

public:
	EFunctionFlags GetFunctionFlags();
	bool HasFlags(EFunctionFlags Flags);

	std::string StringifyFlags();
	std::string GetParamStructName();
};

class UEClass : public UEStruct
{
	using UEStruct::UEStruct;

public:
	EClassCastFlags GetCastFlags();
	bool IsType(EClassCastFlags TypeFlag);
	bool HasType(UEClass TypeClass);
	UEObject GetDefaultObject();

	UEFunction GetFunction(const std::string& ClassName, const std::string& FuncName);
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

	void* GetAddress();

	std::pair<UEClass, UEFFieldClass> GetClass();

	template<typename UEType>
	UEType Cast();

	bool IsA(EClassCastFlags TypeFlags);

	FName GetFName();
	int32 GetArrayDim();
	int32 GetSize();
	int32 GetOffset();
	EPropertyFlags GetPropertyFlags();
	EMappingsTypeFlags GetMappingType();
	bool HasPropertyFlags(EPropertyFlags PropertyFlag);

	UEObject GetOutermost();

	std::string GetName();
	std::string GetValidName();

	std::string GetCppType();

	std::string StringifyFlags();
};

class UEByteProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEEnum GetEnum();

	std::string GetCppType();
};

class UEBoolProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	uint8 GetFieldMask();
	uint8 GetBitIndex();
	bool IsNativeBool();

	std::string GetCppType();
};

class UEObjectProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEClass GetPropertyClass();

	std::string GetCppType();
};

class UEClassProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	UEClass GetMetaClass();

	std::string GetCppType();
};

class UEWeakObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType();
};

class UELazyObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType();
};

class UESoftObjectProperty : public UEObjectProperty
{
	using UEObjectProperty::UEObjectProperty;

public:
	std::string GetCppType();
};

class UESoftClassProperty : public UEClassProperty
{
	using UEClassProperty::UEClassProperty;

public:
	std::string GetCppType();
};

class UEStructProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEStruct GetUnderlayingStruct();

	std::string GetCppType();
};

class UEArrayProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetInnerProperty();

	std::string GetCppType();
};

class UEMapProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetKeyProperty();
	UEProperty GetValueProperty();

	std::string GetCppType();
};

class UESetProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetElementProperty();

	std::string GetCppType();
};

class UEEnumProperty : public UEProperty
{
	using UEProperty::UEProperty;

public:
	UEProperty GetUnderlayingProperty();
	UEEnum GetEnum();

	std::string GetCppType();
};
