#include <format>

#include "UnrealObjects.h"
#include "Offsets.h"
#include "ObjectArray.h"

std::unordered_map<int32, std::string> UEEnum::BigEnums;
std::unordered_map<std::string, uint32> UEProperty::UnknownProperties;
std::unordered_map<int32, uint32> UEStruct::StructSizes;

void* UEFFieldClass::GetAddress()
{
	return Class;
}

EFieldClassID UEFFieldClass::GetId()
{
	return *reinterpret_cast<EFieldClassID*>(Class + Off::FFieldClass::Id);
}

EClassCastFlags UEFFieldClass::GetCastFlags()
{
	return *reinterpret_cast<EClassCastFlags*>(Class + Off::FFieldClass::CastFlags);
}

EClassFlags UEFFieldClass::GetClassFlags()
{
	return *reinterpret_cast<EClassFlags*>(Class + Off::FFieldClass::ClassFlags);
}

UEFFieldClass UEFFieldClass::GetSuper()
{
	return UEFFieldClass(*reinterpret_cast<void**>(Class + Off::FFieldClass::SuperClass));
}

FName UEFFieldClass::GetFName()
{
	return *reinterpret_cast<FName*>(Class + Off::FFieldClass::Name);
}

bool UEFFieldClass::IsType(EClassCastFlags Flags)
{
	return GetCastFlags() & Flags;
}

std::string UEFFieldClass::GetName()
{
	return Class ? GetFName().ToString() : "None";
}

void* UEFField::GetAddress()
{
	return Field;
}

UEFField::FFieldObjectStruct UEFField::GetOwner()
{
	return* reinterpret_cast<FFieldObjectStruct*>(Field + Off::FField::Owner);
}

EObjectFlags UEFField::GetFlags()
{
	return *reinterpret_cast<EObjectFlags*>(Field + Off::FField::Flags);
}

UEFFieldClass UEFField::GetClass()
{
	return UEFFieldClass(*reinterpret_cast<void**>(Field + Off::FField::Class));
}

FName UEFField::GetFName()
{
	return *reinterpret_cast<FName*>(Field + Off::FField::Name);
}

UEFField UEFField::GetNext()
{
	return UEFField(*reinterpret_cast<void**>(Field + Off::FField::Next));
}

template<typename UEType>
UEType UEFField::Cast() const
{
	return UEType(Field);
}

bool UEFField::IsOwnerUObject()
{
	bool IsUObject = *reinterpret_cast<bool*>(Field + Off::FField::Owner + 0x8);

	if (IsUObject)
		return true;
	
	return false;
}

bool UEFField::IsA(EClassCastFlags Flags)
{
	return (Flags != EClassCastFlags::None ? GetClass().IsType(Flags) : true);
}

std::string UEFField::GetName()
{
	return Field ? GetFName().ToString() : "None";
}

std::string UEFField::GetValidName()
{
	std::string Name = GetName();

	char& FirstChar = Name[0];

	if (Name == "bool")
	{
		FirstChar -= 0x20;

		return Name;
	}

	if (Name == "TRUE")
		return "TURR";

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

std::string UEFField::GetCppName()
{
	static UEClass ActorClass = ObjectArray::FindClassFast("Actor");

	std::string Temp = GetValidName();

	if (IsA(EClassCastFlags::Class))
	{
		if (Cast<UEClass>().HasType(ActorClass))
		{
			return 'A' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

UEFField::operator bool()
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

EObjectFlags UEObject::GetFlags()
{
	return *reinterpret_cast<EObjectFlags*>(Object + Off::UObject::Flags);
}

int32 UEObject::GetIndex()
{
	return *reinterpret_cast<int32*>(Object + Off::UObject::Index);
}

UEClass UEObject::GetClass()
{
	return UEClass(*reinterpret_cast<void**>(Object + Off::UObject::Class));
}

FName UEObject::GetFName()
{
	return *reinterpret_cast<FName*>(Object + Off::UObject::Name);
}

UEObject UEObject::GetOuter()
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UObject::Outer));
}

bool UEObject::HasAnyFlags(EObjectFlags Flags)
{
	return GetFlags() & Flags;
}

template<typename UEType>
UEType UEObject::Cast() const
{
	return UEType(Object);
}

bool UEObject::IsA(EClassCastFlags TypeFlags)
{
	return (TypeFlags != EClassCastFlags::None ? GetClass().IsType(TypeFlags) : true);
}

UEObject UEObject::GetOutermost()
{
	UEObject Outermost;

	for (UEObject Outer = *this; Outer; Outer = Outer.GetOuter())
	{
		Outermost = Outer;
	}
	return Outermost;
}

std::string UEObject::GetName()
{
	return Object ? GetFName().ToString() : "None";
}

std::string UEObject::GetValidName()
{
	std::string Name = GetName();

	char& FirstChar = Name[0];

	if (Name == "bool")
	{
		FirstChar -= 0x20;

		return Name;
	}

	if (Name == "TRUE")
		return "TURR";

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

std::string UEObject::GetCppName()
{
	static UEClass ActorClass = ObjectArray::FindClassFast("Actor");

	std::string Temp = GetValidName();

	if (this->IsA(EClassCastFlags::Class))
	{
		if (this->Cast<UEClass>().HasType(ActorClass))
		{
			return 'A' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

std::string UEObject::GetFullName()
{
	if (GetClass())
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

UEObject::operator bool()
{
	// if an object is 0x10000F000 it passes the nullptr check
	return Object != nullptr && reinterpret_cast<void*>(Object + Off::UObject::Class) != nullptr;
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

UEField UEField::GetNext()
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UField::Next));
}

bool UEField::IsNextValid()
{
	return (bool)GetNext();
}

TArray<TPair<FName, int64>>& UEEnum::GetNameValuePairs()
{
	return *reinterpret_cast<TArray<TPair<FName, int64>>*>(Object + Off::UEnum::Names);
}

std::string UEEnum::GetSingleName(int32 Index)
{
	return GetNameValuePairs()[Index].First.ToString();
}

std::vector<std::string> UEEnum::GetAllNames()
{
	auto& Names = GetNameValuePairs();

	std::vector<std::string> RetVec(Names.Num());

	for (int i = 0; i < Names.Num(); i++)
	{
		RetVec.push_back(Names[i].First.ToString());
	}

	return RetVec;
}

std::string UEEnum::GetEnumTypeAsStr()
{
	std::string Temp = GetName();

	return "enum class " + (Temp[0] == 'E' ? Temp : 'E' + Temp);
}

std::string UEEnum::UNSAFEGetCPPType()
{
	return (*reinterpret_cast<FString*>(Object + Off::UEnum::Names - 0x10)).ToString();
}

std::string UEEnum::UNSAFEGetDeclarationType()
{
	int f = *reinterpret_cast<int*>(Object + Off::UEnum::Names + 0x10);

	if (f == 0)
	{
		return "Regular";
	}
	else if (f == 1)
	{
		return "Namespaced";
	}
	else if (f == 2)
	{
		return "EnumClass";
	}
}

UEStruct UEStruct::GetSuper()
{
	return UEStruct(*reinterpret_cast<void**>(Object + Off::UStruct::SuperStruct));
}

UEField UEStruct::GetChild()
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UStruct::Children));
}

UEFField UEStruct::GetChildProperties()
{
	return UEFField(*reinterpret_cast<void**>(Object + Off::UStruct::ChildProperties));
}

int32 UEStruct::GetStructSize()
{
	return *reinterpret_cast<int32*>(Object + Off::UStruct::Size);
}

std::vector<UEProperty> UEStruct::GetProperties()
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

bool UEStruct::HasMembers()
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

EClassCastFlags UEClass::GetCastFlags()
{
	return *reinterpret_cast<EClassCastFlags*>(Object + Off::UClass::ClassFlags);
}

bool UEClass::IsType(EClassCastFlags TypeFlag)
{
	return GetCastFlags() & TypeFlag;
}

bool UEClass::HasType(UEClass TypeClass)
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

UEObject UEClass::GetDefaultObject()
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UClass::ClassDefaultObject));
}

UEFunction UEClass::GetFunction(const std::string& ClassName, const std::string& FuncName)
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

EFunctionFlags UEFunction::GetFunctionFlags()
{
	return *reinterpret_cast<EFunctionFlags*>(Object + Off::UFunction::FunctionFlags);
}

bool UEFunction::HasFlags(EFunctionFlags FuncFlags)
{
	return GetFunctionFlags() & FuncFlags;
}

std::string UEFunction::StringifyFlags() 
{
	return StringifyFunctionFlags(GetFunctionFlags());
}

std::string UEFunction::GetParamStructName()
{
	return GetOuter().GetCppName() + "_" + GetValidName() + "_Params";
}

void* UEProperty::GetAddress()
{
	return Base;
}

std::pair<UEClass, UEFField> UEProperty::GetClass()
{
	if (Settings::Internal::bUseFProperty)
	{
		return std::pair<UEClass, UEFField>(UEClass(nullptr), UEFField(Base));
	}

	return std::pair<UEClass, UEFField>(UEClass(Base + Off::UObject::Class), UEFField(nullptr));
}
	
template<typename UEType>
inline UEType UEProperty::Cast()
{
	return UEType(Base);
}

bool UEProperty::IsA(EClassCastFlags TypeFlags)
{
	if (GetClass().first)
	{
		return GetClass().first.IsA(TypeFlags);
	}

	return GetClass().second.IsA(TypeFlags);
}

FName UEProperty::GetFName()
{
	if (Settings::Internal::bUseFProperty)
	{
		return *reinterpret_cast<FName*>(Base + Off::FField::Name);
	}

	return *reinterpret_cast<FName*>(Base + Off::UObject::Name);
}

int32 UEProperty::GetSize()
{
	return *reinterpret_cast<int32*>(Base + Off::UProperty::ElementSize);
}

int32 UEProperty::GetOffset()
{
	return *reinterpret_cast<int32*>(Base + Off::UProperty::Offset_Internal);
}

EPropertyFlags UEProperty::GetPropertyFlags()
{
	return *reinterpret_cast<EPropertyFlags*>(Base + Off::UProperty::PropertyFlags);
}

bool UEProperty::HasPropertyFlags(EPropertyFlags PropertyFlag)
{
	return GetPropertyFlags() & PropertyFlag;
}

std::string UEProperty::GetName()
{
	return Base ? GetFName().ToString() : "None";
}

std::string UEProperty::GetValidName()
{
	std::string Name = GetName();

	char& FirstChar = Name[0];

	if (Name == "bool")
	{
		FirstChar -= 0x20;

		return Name;
	}

	if (Name == "TRUE")
		return "TURR";

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

std::string UEProperty::GetCppType()
{
	EClassCastFlags TypeFlags = this->GetClass().first ? this->GetClass().first.GetCastFlags() : this->GetClass().second.GetClass().GetCastFlags();

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
	else if (TypeFlags &  EClassCastFlags::UInt16Property)
	{
		return "int16";
	}
	else if (TypeFlags &  EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (TypeFlags &  EClassCastFlags::UInt64Property)
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
	else
	{
		std::string CppName = GetClass().first ? GetClass().first.GetCppName() + "_" : GetClass().second.GetCppName() + "_";

		UnknownProperties.insert({ CppName, GetSize() });

		return CppName;
	}
}

std::string UEProperty::StringifyFlags()
{
	return StringifyPropertyFlags(GetPropertyFlags());
}

UEEnum UEByteProperty::GetEnum()
{
	return UEEnum(*reinterpret_cast<void**>(Base + Off::UByteProperty::Enum));
}

std::string UEByteProperty::GetCppType()
{
	if (UEEnum Enum = GetEnum())
	{
		return Enum.GetEnumTypeAsStr();
	}

	return "uint8";
}

uint8 UEBoolProperty::GetFieldMask()
{
	return reinterpret_cast<Off::UBoolProperty::UBoolPropertyBase*>(Base + Off::UBoolProperty::Base)->FieldMask;
}

uint8 UEBoolProperty::GetBitIndex()
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

bool UEBoolProperty::IsNativeBool()
{
	return reinterpret_cast<Off::UBoolProperty::UBoolPropertyBase*>(Base + Off::UBoolProperty::Base)->FieldMask == 0xFF;
}

std::string UEBoolProperty::GetCppType()
{
	return IsNativeBool() ? "bool" : "uint8";
}

UEClass UEObjectProperty::GetPropertyClass()
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::UObjectProperty::PropertyClass));
}

std::string UEObjectProperty::GetCppType()
{
	return std::format("class {}*", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

UEClass UEClassProperty::GetMetaClass()
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::UClassProperty::MetaClass));
}

std::string UEClassProperty::GetCppType()
{
	return HasPropertyFlags(EPropertyFlags::UObjectWrapper) ? std::format("TSubclassOf<class {}>", GetMetaClass().GetCppName()) : "class UClass*";
}

std::string UEWeakObjectProperty::GetCppType()
{
	return std::format("TWeakObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UELazyObjectProperty::GetCppType()
{
	return std::format("TLazyObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UESoftObjectProperty::GetCppType()
{
	return std::format("TSoftObjectPtr<class {}>", GetPropertyClass().GetCppName());
}

std::string UESoftClassProperty::GetCppType()
{
	return std::format("TSoftClassPtr<class {}>", GetMetaClass() ? GetMetaClass().GetCppName() : GetPropertyClass().GetCppName());
}

UEStruct UEStructProperty::GetUnderlayingStruct()
{
	return UEStruct(*reinterpret_cast<void**>(Base + Off::UStructProperty::Struct));
}

std::string UEStructProperty::GetCppType()
{
	return std::format("struct {}", GetUnderlayingStruct().GetCppName());
}

UEProperty UEArrayProperty::GetInnerProperty()
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::UArrayProperty::Inner));
}

std::string UEArrayProperty::GetCppType()
{
	return std::format("TArray<{}>", GetInnerProperty().GetCppType());
}

UEProperty UEMapProperty::GetKeyProperty()
{
	return UEProperty(reinterpret_cast<Off::UMapProperty::UMapPropertyBase*>(Base + Off::UMapProperty::Base)->KeyProperty);
}

UEProperty UEMapProperty::GetValueProperty()
{
	return UEProperty(reinterpret_cast<Off::UMapProperty::UMapPropertyBase*>(Base + Off::UMapProperty::Base)->ValueProperty);
}

std::string UEMapProperty::GetCppType()
{
	return std::format("TMap<{}, {}>", GetKeyProperty().GetCppType(), GetValueProperty().GetCppType());
}

UEProperty UESetProperty::GetElementProperty()
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::USetProperty::ElementProp));
}

std::string UESetProperty::GetCppType()
{
	return std::format("TSet<{}>", GetElementProperty().GetCppType());
}

UEProperty UEEnumProperty::GetUnderlayingProperty()
{
	return UEProperty(reinterpret_cast<Off::UEnumProperty::UEnumPropertyBase*>(Base + Off::UEnumProperty::Base)->UnderlayingProperty);
}

UEEnum UEEnumProperty::GetEnum()
{
	return UEEnum(reinterpret_cast<Off::UEnumProperty::UEnumPropertyBase*>(Base + Off::UEnumProperty::Base)->Enum);
}

std::string UEEnumProperty::GetCppType()
{	
	return GetEnum().GetEnumTypeAsStr();
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

	PDummy.Cast<UEField>();
	PDummy.Cast<UEProperty>();

	PDummy.Cast<UEField&>();
	PDummy.Cast<UEProperty&>();

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
}
