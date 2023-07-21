#include <format>

#include "UnrealObjects.h"
#include "Offsets.h"
#include "ObjectArray.h"

std::unordered_map<int32, std::string> UEEnum::BigEnums;
std::unordered_map<std::string, uint32> UEProperty::UnknownProperties;
std::unordered_map<int32, uint32> UEStruct::StructSizes;

inline std::string MakeNameValid(std::string&& Name)
{
	char& FirstChar = Name[0];

	if (Name == "bool")
	{
		FirstChar -= 0x20;

		return Name;
	}

	if (Name == "TRUE")
		return "TURR";

	if (Name == "FALSE")
		return "FLASE";

	if (FirstChar <= '9' && FirstChar >= '0')
		Name = '_' + Name;

	// this way I don't need to bother checking for c++ types (except bool) like int in the names
	if ((FirstChar <= 'z' && FirstChar >= 'a') && FirstChar != 'b')
		FirstChar = FirstChar - 0x20;

	for (char& c : Name)
	{
		if (c != '_' && !((c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A') || (c <= '9' && c >= '0')))
		{
			c = '_';
		}
	}

	return Name;
}


void* UEFFieldClass::GetAddress()
{
	return Class;
}

EFieldClassID UEFFieldClass::GetId() const
{
	return *reinterpret_cast<EFieldClassID*>(Class + Off::FFieldClass::Id);
}

EClassCastFlags UEFFieldClass::GetCastFlags() const
{
	return *reinterpret_cast<EClassCastFlags*>(Class + Off::FFieldClass::CastFlags);
}

EClassFlags UEFFieldClass::GetClassFlags() const
{
	return *reinterpret_cast<EClassFlags*>(Class + Off::FFieldClass::ClassFlags);
}

UEFFieldClass UEFFieldClass::GetSuper() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Class + Off::FFieldClass::SuperClass));
}

FName UEFFieldClass::GetFName() const
{
	return FName(Class + Off::FFieldClass::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

bool UEFFieldClass::IsType(EClassCastFlags Flags) const
{
	return (Flags != EClassCastFlags::None ? (GetCastFlags() & Flags) : true);
}

std::string UEFFieldClass::GetName() const
{
	return Class ? GetFName().ToString() : "None";
}

std::string UEFFieldClass::GetValidName() const
{
	return MakeNameValid(GetName());
}

std::string UEFFieldClass::GetCppName() const
{
	// This is evile dark magic code which shouldn't exist
	return "F" + GetValidName();
}

void* UEFField::GetAddress()
{
	return Field;
}

EObjectFlags UEFField::GetFlags() const
{
	return *reinterpret_cast<EObjectFlags*>(Field + Off::FField::Flags);
}

class UEObject UEFField::GetOwnerAsUObject() const
{
	if (IsOwnerUObject())
	{
		if (Settings::Internal::bUseMaskForFieldOwner)
			return (void*)(*reinterpret_cast<uintptr_t*>(Field + Off::FField::Owner) & ~0x1ull);

		return *reinterpret_cast<void**>(Field + Off::FField::Owner);
	}

	return nullptr;
}

class UEFField UEFField::GetOwnerAsFField() const
{
	if (!IsOwnerUObject())
		return *reinterpret_cast<void**>(Field + Off::FField::Owner);

	return nullptr;
}

class UEObject UEFField::GetOwnerUObject() const
{
	UEFField Field = *this;

	while (!Field.IsOwnerUObject() && Field.GetOwnerAsFField())
	{
		Field = Field.GetOwnerAsFField();
	}

	return Field.GetOwnerAsUObject();
}

class UEObject UEFField::GetOutermost() const
{
	UEObject OwnerUObject = GetOwnerUObject();
	return OwnerUObject.GetOutermost();
}

UEFFieldClass UEFField::GetClass() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Field + Off::FField::Class));
}

FName UEFField::GetFName() const
{
	return FName(Field + Off::FField::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

UEFField UEFField::GetNext() const
{
	return UEFField(*reinterpret_cast<void**>(Field + Off::FField::Next));
}

template<typename UEType>
UEType UEFField::Cast() const
{
	return UEType(Field);
}

bool UEFField::IsOwnerUObject() const
{
	if (Settings::Internal::bUseMaskForFieldOwner)
	{
		return *reinterpret_cast<uintptr_t*>(Field + Off::FField::Owner) & 0x1;
	}

	return *reinterpret_cast<bool*>(Field + Off::FField::Owner + 0x8);
}

bool UEFField::IsA(EClassCastFlags Flags) const
{
	return (Flags != EClassCastFlags::None ? GetClass().IsType(Flags) : true);
}

std::string UEFField::GetName() const
{
	return Field ? GetFName().ToString() : "None";
}

std::string UEFField::GetValidName() const
{
	return MakeNameValid(GetName());
}

std::string UEFField::GetCppName() const
{
	static UEClass ActorClass = ObjectArray::FindClassFast("Actor");
	static UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

	std::string Temp = GetValidName();

	if (IsA(EClassCastFlags::Class))
	{
		if (Cast<UEClass>().HasType(ActorClass)) 
		{
			return 'A' + Temp;
		}
		else if (Cast<UEClass>().HasType(InterfaceClass)) 
		{
			return 'I' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

UEFField::operator bool() const
{
	return Field != nullptr && reinterpret_cast<void*>(Field + Off::FField::Class) != nullptr;
}

bool UEFField::operator==(const UEFField& Other) const
{
	return Field == Other.Field;
}

bool UEFField::operator!=(const UEFField& Other) const
{
	return Field != Other.Field;
}

void(*UEObject::PE)(void*, void*, void*) = nullptr;

void* UEObject::GetAddress()
{
	return Object;
}

EObjectFlags UEObject::GetFlags() const
{
	return *reinterpret_cast<EObjectFlags*>(Object + Off::UObject::Flags);
}

int32 UEObject::GetIndex() const
{
	return *reinterpret_cast<int32*>(Object + Off::UObject::Index);
}

UEClass UEObject::GetClass() const
{
	return UEClass(*reinterpret_cast<void**>(Object + Off::UObject::Class));
}

FName UEObject::GetFName() const
{
	return FName(Object + Off::UObject::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

UEObject UEObject::GetOuter() const
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UObject::Outer));
}

bool UEObject::HasAnyFlags(EObjectFlags Flags) const
{
	return GetFlags() & Flags;
}

template<typename UEType>
UEType UEObject::Cast()
{
	return UEType(Object);
}

template<typename UEType>
const UEType UEObject::Cast() const
{
	return UEType(Object);
}

bool UEObject::IsA(EClassCastFlags TypeFlags) const
{
	return (TypeFlags != EClassCastFlags::None ? GetClass().IsType(TypeFlags) : true);
}

UEObject UEObject::GetOutermost() const
{
	UEObject Outermost;

	for (UEObject Outer = *this; Outer; Outer = Outer.GetOuter())
	{
		Outermost = Outer;
	}
	return Outermost;
}

std::string UEObject::GetName() const
{
	return Object ? GetFName().ToString() : "None";
}

std::string UEObject::GetValidName() const
{
	return MakeNameValid(GetName());
}

std::string UEObject::GetCppName() const
{
	static UEClass ActorClass = ObjectArray::FindClassFast("Actor");
	static UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

	std::string Temp = GetValidName();

	if (IsA(EClassCastFlags::Class))
	{
		if (Cast<UEClass>().HasType(ActorClass))
		{
			return 'A' + Temp;
		}
		else if (Cast<UEClass>().HasType(InterfaceClass))
		{
			return 'I' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

std::string UEObject::GetFullName() const
{
	if (*this)
	{
		std::string Temp;

		for (UEObject Outer = GetOuter(); Outer; Outer = Outer.GetOuter())
		{
			Temp = Outer.GetName() + "." + Temp;
		}

		std::string Name = GetClass().GetName();
		Name += " ";
		Name += Temp;
		Name += GetName();

		return Name;
	}

	return "None";
}

UEObject::operator bool() const
{
	// if an object is 0x10000F000 it passes the nullptr check
	return Object != nullptr && reinterpret_cast<void*>(Object + Off::UObject::Class) != nullptr;
}

UEObject::operator uint8*()
{
	return Object;
}

bool UEObject::operator==(const UEObject& Other) const
{
	return Object == Other.Object;
}

bool UEObject::operator!=(const UEObject& Other) const
{
	return Object != Other.Object;
}

void UEObject::ProcessEvent(UEFunction Func, void* Params)
{
	void** VFT = *reinterpret_cast<void***>(GetAddress());

	void(*Prd)(void*, void*, void*) = decltype(Prd)(VFT[Off::InSDK::PEIndex]);

	Prd(Object, Func.GetAddress(), Params);
}

UEField UEField::GetNext() const
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UField::Next));
}

bool UEField::IsNextValid() const
{
	return (bool)GetNext();
}

std::vector<TPair<FName, int64>> UEEnum::GetNameValuePairs() const
{
	struct Name08Byte { uint8 Pad[0x08]; };
	struct Name16Byte { uint8 Pad[0x10]; };

	std::vector<TPair<FName, int64>> Ret;

	if (!Settings::Internal::bIsEnumNameOnly)
	{
		if (Settings::Internal::bUseCasePreservingName)
		{
			auto& Names = *reinterpret_cast<TArray<TPair<Name16Byte, int64>>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i].First), Names[i].Second });
			}
		}
		else
		{
			auto& Names = *reinterpret_cast<TArray<TPair<Name08Byte, int64>>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i].First), Names[i].Second });
			}
		}
	}
	else
	{
		auto& NameOnly = *reinterpret_cast<TArray<FName>*>(Object + Off::UEnum::Names);

		if (Settings::Internal::bUseCasePreservingName)
		{
			auto& Names = *reinterpret_cast<TArray<Name16Byte>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i]), i });
			}
		}
		else
		{
			auto& Names = *reinterpret_cast<TArray<Name08Byte>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i]), i });
			}
		}
	}

	return Ret;
}

std::string UEEnum::GetSingleName(int32 Index) const
{
	return GetNameValuePairs()[Index].First.ToString();
}

std::string UEEnum::GetEnumTypeAsStr() const
{
	std::string Temp = GetName();

	return "enum class " + (Temp[0] == 'E' ? Temp : 'E' + Temp);
}


UEStruct UEStruct::GetSuper() const
{
	return UEStruct(*reinterpret_cast<void**>(Object + Off::UStruct::SuperStruct));
}

UEField UEStruct::GetChild() const
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UStruct::Children));
}

UEFField UEStruct::GetChildProperties() const
{
	return UEFField(*reinterpret_cast<void**>(Object + Off::UStruct::ChildProperties));
}

int32 UEStruct::GetStructSize() const
{
	return *reinterpret_cast<int32*>(Object + Off::UStruct::Size);
}

std::vector<UEProperty> UEStruct::GetProperties() const
{
	std::vector<UEProperty> Properties;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Property))
			{
				Properties.push_back(Field.Cast<UEProperty>());
			}
		}

		return Properties;
	}

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(EClassCastFlags::Property))
		{
			Properties.push_back(Field.Cast<UEProperty>());
		}
	}

	return Properties;
}

bool UEStruct::HasMembers() const
{
	if (!Object)
		return false;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Property))
			{
				return true;
			}
		}
	}
	else
	{
		for (UEField F = GetChild(); F; F = F.GetNext())
		{
			if (F.IsA(EClassCastFlags::Property))
			{
				return true;
			}
		}
	}

	return false;
}

EClassCastFlags UEClass::GetCastFlags() const
{
	return *reinterpret_cast<EClassCastFlags*>(Object + Off::UClass::CastFlags);
}

bool UEClass::IsType(EClassCastFlags TypeFlag) const
{
	return (TypeFlag != EClassCastFlags::None ? (GetCastFlags() & TypeFlag) : true);
}

bool UEClass::HasType(UEClass TypeClass) const
{
	if (TypeClass == nullptr)
		return false;

	for (UEStruct S = *this; S; S = S.GetSuper())
	{
		if (S == TypeClass)
			return true;
	}

	return false;
}

UEObject UEClass::GetDefaultObject() const
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UClass::ClassDefaultObject));
}

UEFunction UEClass::GetFunction(const std::string& ClassName, const std::string& FuncName) const
{
	for (UEStruct Struct = *this; Struct; Struct = Struct.GetSuper())
	{
		if (Struct.GetName() == ClassName)
		{
			for (UEField Field = Struct.GetChild(); Field; Field = Field.GetNext())
			{
				if (Field.IsA(EClassCastFlags::Function) && Field.GetName() == FuncName)
				{
					return Field.Cast<UEFunction>();
				}	
			}
		}
	}

	return nullptr;
}

EFunctionFlags UEFunction::GetFunctionFlags() const
{
	return *reinterpret_cast<EFunctionFlags*>(Object + Off::UFunction::FunctionFlags);
}

bool UEFunction::HasFlags(EFunctionFlags FuncFlags) const
{
	return GetFunctionFlags() & FuncFlags;
}

std::string UEFunction::StringifyFlags()  const
{
	return StringifyFunctionFlags(GetFunctionFlags());
}

std::string UEFunction::GetParamStructName() const
{
	return GetOuter().GetCppName() + "_" + GetValidName() + "_Params";
}

void* UEProperty::GetAddress()
{
	return Base;
}

std::pair<UEClass, UEFFieldClass> UEProperty::GetClass() const
{
	if (Settings::Internal::bUseFProperty)
	{
		return { UEClass(0), UEFField(Base).GetClass() };
	}

	return { UEObject(Base).GetClass(), UEFFieldClass(0) };
}

template<typename UEType>
UEType UEProperty::Cast()
{
	return UEType(Base);
}

template<typename UEType>
const UEType UEProperty::Cast() const
{
	return UEType(Base);
}

bool UEProperty::IsA(EClassCastFlags TypeFlags) const
{
	if (GetClass().first)
		return GetClass().first.IsType(TypeFlags);
	
	return GetClass().second.IsType(TypeFlags);
}

bool UEProperty::IsTypeSupported() const
{
	EClassCastFlags PropTypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	return static_cast<std::underlying_type_t<EClassCastFlags>>(PropTypeFlags) & static_cast<std::underlying_type_t<EClassCastFlags>>(GetSupportedProperties());
}

FName UEProperty::GetFName() const
{
	if (Settings::Internal::bUseFProperty)
	{
		return FName(Base + Off::FField::Name); //Not the real FName, but a wrapper which holds the address of a FName
	}

	return FName(Base + Off::UObject::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

int32 UEProperty::GetArrayDim() const
{
	return *reinterpret_cast<int32*>(Base + Off::UProperty::ArrayDim);
}

int32 UEProperty::GetSize() const
{
	return *reinterpret_cast<int32*>(Base + Off::UProperty::ElementSize);
}

int32 UEProperty::GetOffset() const
{
	return *reinterpret_cast<int32*>(Base + Off::UProperty::Offset_Internal);
}

EPropertyFlags UEProperty::GetPropertyFlags() const
{
	return *reinterpret_cast<EPropertyFlags*>(Base + Off::UProperty::PropertyFlags);
}

EMappingsTypeFlags UEProperty::GetMappingType() const
{
	EClassCastFlags Flags = GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags();

	return EPropertyFlagsToMappingFlags(Flags);
}

bool UEProperty::HasPropertyFlags(EPropertyFlags PropertyFlag) const
{
	return GetPropertyFlags() & PropertyFlag;
}

UEObject UEProperty::GetOutermost() const
{
	return Settings::Internal::bUseFProperty ? UEFField(Base).GetOutermost() : UEObject(Base).GetOutermost();
}

std::string UEProperty::GetName() const
{
	return Base ? GetFName().ToString() : "None";
}

std::string UEProperty::GetValidName() const
{
	return MakeNameValid(GetName());
}

std::string UEProperty::GetCppType() const
{
	EClassCastFlags TypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	if (TypeFlags & EClassCastFlags::ByteProperty)
	{
		return Cast<UEByteProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (TypeFlags &  EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (TypeFlags &  EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (TypeFlags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (TypeFlags &  EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (TypeFlags &  EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (TypeFlags &  EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (TypeFlags &  EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (TypeFlags &  EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (TypeFlags &  EClassCastFlags::ClassProperty)
	{
		return Cast<UEClassProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (TypeFlags &  EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (TypeFlags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (TypeFlags &  EClassCastFlags::BoolProperty)
	{
		return Cast<UEBoolProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::StructProperty)
	{
		return Cast<UEStructProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::ArrayProperty)
	{
		return Cast<UEArrayProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::WeakObjectProperty)
	{
		return Cast<UEWeakObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::LazyObjectProperty)
	{
		return Cast<UELazyObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::SoftClassProperty)
	{
		return Cast<UESoftClassProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::SoftObjectProperty)
	{
		return Cast<UESoftObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::ObjectProperty)
	{
		return Cast<UEObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::MapProperty)
	{
		return Cast<UEMapProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::SetProperty)
	{
		return Cast<UESetProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::EnumProperty)
	{
		return Cast<UEEnumProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::InterfaceProperty) 
	{
		return Cast<UEInterfaceProperty>().GetCppType();
	}
	else
	{
		return Cast<UEUnknonProperty>().GetCppType();
	}
}

std::string UEProperty::StringifyFlags() const
{
	return StringifyPropertyFlags(GetPropertyFlags());
}

UEEnum UEByteProperty::GetEnum() const
{
	return UEEnum(*reinterpret_cast<void**>(Base + Off::UByteProperty::Enum));
}

std::string UEByteProperty::GetCppType() const
{
	if (UEEnum Enum = GetEnum())
	{
		return Enum.GetEnumTypeAsStr();
	}

	return "uint8";
}

uint8 UEBoolProperty::GetFieldMask() const
{
	return reinterpret_cast<Off::UBoolProperty::UBoolPropertyBase*>(Base + Off::UBoolProperty::Base)->FieldMask;
}

uint8 UEBoolProperty::GetBitIndex() const
{
	uint8 FieldMask = GetFieldMask();
	
	if (FieldMask != 0xFF)
	{
		if (FieldMask == 0x01) { return 1; }
		if (FieldMask == 0x02) { return 2; }
		if (FieldMask == 0x04) { return 3; }
		if (FieldMask == 0x08) { return 4; }
		if (FieldMask == 0x10) { return 5; }
		if (FieldMask == 0x20) { return 6; }
		if (FieldMask == 0x40) { return 7; }
		if (FieldMask == 0x80) { return 8; }
	}

	return 0xFF;
}

bool UEBoolProperty::IsNativeBool() const
{
	return reinterpret_cast<Off::UBoolProperty::UBoolPropertyBase*>(Base + Off::UBoolProperty::Base)->FieldMask == 0xFF;
}

std::string UEBoolProperty::GetCppType() const
{
	return IsNativeBool() ? "bool" : "uint8";
}

UEClass UEObjectProperty::GetPropertyClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::UObjectProperty::PropertyClass));
}

std::string UEObjectProperty::GetCppType() const
{
	return std::format("class {}*", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

UEClass UEClassProperty::GetMetaClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::UClassProperty::MetaClass));
}

std::string UEClassProperty::GetCppType() const
{
	return HasPropertyFlags(EPropertyFlags::UObjectWrapper) ? std::format("TSubclassOf<class {}>", GetMetaClass().GetCppName()) : "class UClass*";
}

std::string UEWeakObjectProperty::GetCppType() const
{
	return std::format("TWeakObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UELazyObjectProperty::GetCppType() const
{
	return std::format("TLazyObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UESoftObjectProperty::GetCppType() const
{
	//Debugging cause why not?
	//printf("GetPropertyClass() -> 0x%X\n", GetPropertyClass().GetAddress());

	if (!GetPropertyClass())
		return "TSoftObjectPtr<class UObject>";

	return std::format("TSoftObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UESoftClassProperty::GetCppType() const
{
	return std::format("TSoftClassPtr<class {}>", GetMetaClass() ? GetMetaClass().GetCppName() : GetPropertyClass().GetCppName());
}

std::string UEInterfaceProperty::GetCppType() const
{
	return std::format("TScriptInterface<class {}>", GetPropertyClass().GetCppName());
}

UEStruct UEStructProperty::GetUnderlayingStruct() const
{
	return UEStruct(*reinterpret_cast<void**>(Base + Off::UStructProperty::Struct));
}

std::string UEStructProperty::GetCppType() const
{
	return std::format("struct {}", GetUnderlayingStruct().GetCppName());
}

UEProperty UEArrayProperty::GetInnerProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::UArrayProperty::Inner));
}

std::string UEArrayProperty::GetCppType() const
{
	return std::format("TArray<{}>", GetInnerProperty().GetCppType());
}

UEProperty UEMapProperty::GetKeyProperty() const
{
	return UEProperty(reinterpret_cast<Off::UMapProperty::UMapPropertyBase*>(Base + Off::UMapProperty::Base)->KeyProperty);
}

UEProperty UEMapProperty::GetValueProperty() const
{
	return UEProperty(reinterpret_cast<Off::UMapProperty::UMapPropertyBase*>(Base + Off::UMapProperty::Base)->ValueProperty);
}

std::string UEMapProperty::GetCppType() const
{
	return std::format("TMap<{}, {}>", GetKeyProperty().GetCppType(), GetValueProperty().GetCppType());
}

UEProperty UESetProperty::GetElementProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::USetProperty::ElementProp));
}

std::string UESetProperty::GetCppType() const
{
	return std::format("TSet<{}>", GetElementProperty().GetCppType());
}

UEProperty UEEnumProperty::GetUnderlayingProperty() const
{
	return UEProperty(reinterpret_cast<Off::UEnumProperty::UEnumPropertyBase*>(Base + Off::UEnumProperty::Base)->UnderlayingProperty);
}

UEEnum UEEnumProperty::GetEnum() const
{
	return UEEnum(reinterpret_cast<Off::UEnumProperty::UEnumPropertyBase*>(Base + Off::UEnumProperty::Base)->Enum);
}

std::string UEEnumProperty::GetCppType() const
{
	if (GetEnum())
		return GetEnum().GetEnumTypeAsStr();

	return GetUnderlayingProperty().GetCppType();
}

std::string UEUnknonProperty::GetCppType() const
{
	return (GetClass().first ? GetClass().first.GetCppName() : GetClass().second.GetCppName()) + "_";;
}

/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
*
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
void TemplateTypeCreationForUnrealObjects(void)
{
	UEObject Dummy(nullptr);
	UEFField FDummy(nullptr);
	UEProperty PDummy(nullptr);

	const UEObject CDummy(nullptr);
	const UEFField CFDummy(nullptr);
	const UEProperty CPDummy(nullptr);

	FDummy.Cast<UEFField>();
	FDummy.Cast<UEProperty>();
	FDummy.Cast<UEByteProperty>();
	FDummy.Cast<UEBoolProperty>();
	FDummy.Cast<UEObjectProperty>();
	FDummy.Cast<UEClassProperty>();
	FDummy.Cast<UEStructProperty>();
	FDummy.Cast<UEArrayProperty>();
	FDummy.Cast<UEMapProperty>();
	FDummy.Cast<UESetProperty>();
	FDummy.Cast<UEEnumProperty>();
	FDummy.Cast<UEInterfaceProperty>();

	FDummy.Cast<UEFField&>();
	FDummy.Cast<UEProperty&>();
	FDummy.Cast<UEByteProperty&>();
	FDummy.Cast<UEBoolProperty&>();
	FDummy.Cast<UEObjectProperty&>();
	FDummy.Cast<UEClassProperty&>();
	FDummy.Cast<UEStructProperty&>();
	FDummy.Cast<UEArrayProperty&>();
	FDummy.Cast<UEMapProperty&>();
	FDummy.Cast<UESetProperty&>();
	FDummy.Cast<UEEnumProperty&>();
	FDummy.Cast<UEInterfaceProperty&>();

	PDummy.Cast<UEFField>();
	PDummy.Cast<UEProperty>();
	PDummy.Cast<UEByteProperty>();
	PDummy.Cast<UEBoolProperty>();
	PDummy.Cast<UEObjectProperty>();
	PDummy.Cast<UEClassProperty>();
	PDummy.Cast<UEStructProperty>();
	PDummy.Cast<UEArrayProperty>();
	PDummy.Cast<UEMapProperty>();
	PDummy.Cast<UESetProperty>();
	PDummy.Cast<UEEnumProperty>();
	PDummy.Cast<UEInterfaceProperty>();

	PDummy.Cast<UEFField&>();
	PDummy.Cast<UEProperty&>();
	PDummy.Cast<UEByteProperty&>();
	PDummy.Cast<UEBoolProperty&>();
	PDummy.Cast<UEObjectProperty&>();
	PDummy.Cast<UEClassProperty&>();
	PDummy.Cast<UEStructProperty&>();
	PDummy.Cast<UEArrayProperty&>();
	PDummy.Cast<UEMapProperty&>();
	PDummy.Cast<UESetProperty&>();
	PDummy.Cast<UEEnumProperty&>();
	PDummy.Cast<UEInterfaceProperty&>();

	Dummy.Cast<UEFField>();

	Dummy.Cast<UEObject>();
	Dummy.Cast<UEField>();
	Dummy.Cast<UEEnum>();
	Dummy.Cast<UEStruct>();
	Dummy.Cast<UEClass>();
	Dummy.Cast<UEFunction>();
	Dummy.Cast<UEProperty>();
	Dummy.Cast<UEByteProperty>();
	Dummy.Cast<UEBoolProperty>();
	Dummy.Cast<UEObjectProperty>();
	Dummy.Cast<UEClassProperty>();
	Dummy.Cast<UEStructProperty>();
	Dummy.Cast<UEArrayProperty>();
	Dummy.Cast<UEMapProperty>();
	Dummy.Cast<UESetProperty>();
	Dummy.Cast<UEEnumProperty>();
	Dummy.Cast<UEInterfaceProperty>();

	Dummy.Cast<UEObject&>();
	Dummy.Cast<UEField&>();
	Dummy.Cast<UEEnum&>();
	Dummy.Cast<UEStruct&>();
	Dummy.Cast<UEClass&>();
	Dummy.Cast<UEFunction&>();
	Dummy.Cast<UEProperty&>();
	Dummy.Cast<UEByteProperty&>();
	Dummy.Cast<UEBoolProperty&>();
	Dummy.Cast<UEObjectProperty&>();
	Dummy.Cast<UEClassProperty&>();
	Dummy.Cast<UEStructProperty&>();
	Dummy.Cast<UEArrayProperty&>();
	Dummy.Cast<UEMapProperty&>();
	Dummy.Cast<UESetProperty&>();
	Dummy.Cast<UEEnumProperty&>();
	Dummy.Cast<UEInterfaceProperty&>();


	CPDummy.Cast<UEObject>();
	CPDummy.Cast<UEField>();
	CPDummy.Cast<UEProperty>();

	CPDummy.Cast<UEField&>();
	CPDummy.Cast<UEProperty&>();

	CFDummy.Cast<UEFField>();
	CFDummy.Cast<UEProperty>();
	CFDummy.Cast<UEByteProperty>();
	CFDummy.Cast<UEBoolProperty>();
	CFDummy.Cast<UEObjectProperty>();
	CFDummy.Cast<UEClassProperty>();
	CFDummy.Cast<UEStructProperty>();
	CFDummy.Cast<UEArrayProperty>();
	CFDummy.Cast<UEMapProperty>();
	CFDummy.Cast<UESetProperty>();
	CFDummy.Cast<UEEnumProperty>();
	CFDummy.Cast<UEInterfaceProperty>();

	CFDummy.Cast<UEFField&>();
	CFDummy.Cast<UEProperty&>();
	CFDummy.Cast<UEByteProperty&>();
	CFDummy.Cast<UEBoolProperty&>();
	CFDummy.Cast<UEObjectProperty&>();
	CFDummy.Cast<UEClassProperty&>();
	CFDummy.Cast<UEStructProperty&>();
	CFDummy.Cast<UEArrayProperty&>();
	CFDummy.Cast<UEMapProperty&>();
	CFDummy.Cast<UESetProperty&>();
	CFDummy.Cast<UEEnumProperty&>();
	CFDummy.Cast<UEInterfaceProperty&>();

	CDummy.Cast<UEFField>();

	CDummy.Cast<UEObject>();
	CDummy.Cast<UEField>();
	CDummy.Cast<UEEnum>();
	CDummy.Cast<UEStruct>();
	CDummy.Cast<UEClass>();
	CDummy.Cast<UEFunction>();
	CDummy.Cast<UEProperty>();
	CDummy.Cast<UEByteProperty>();
	CDummy.Cast<UEBoolProperty>();
	CDummy.Cast<UEObjectProperty>();
	CDummy.Cast<UEClassProperty>();
	CDummy.Cast<UEStructProperty>();
	CDummy.Cast<UEArrayProperty>();
	CDummy.Cast<UEMapProperty>();
	CDummy.Cast<UESetProperty>();
	CDummy.Cast<UEEnumProperty>();
	CDummy.Cast<UEInterfaceProperty>();

	CDummy.Cast<UEObject&>();
	CDummy.Cast<UEField&>();
	CDummy.Cast<UEEnum&>();
	CDummy.Cast<UEStruct&>();
	CDummy.Cast<UEClass&>();
	CDummy.Cast<UEFunction&>();
	CDummy.Cast<UEProperty&>();
	CDummy.Cast<UEByteProperty&>();
	CDummy.Cast<UEBoolProperty&>();
	CDummy.Cast<UEObjectProperty&>();
	CDummy.Cast<UEClassProperty&>();
	CDummy.Cast<UEStructProperty&>();
	CDummy.Cast<UEArrayProperty&>();
	CDummy.Cast<UEMapProperty&>();
	CDummy.Cast<UESetProperty&>();
	CDummy.Cast<UEEnumProperty&>();
	CDummy.Cast<UEInterfaceProperty&>();
}
