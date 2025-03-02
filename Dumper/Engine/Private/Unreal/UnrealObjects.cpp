
#include <format>

#include "Unreal/UnrealObjects.h"
#include "Unreal/ObjectArray.h"
#include "OffsetFinder/Offsets.h"


void* UEFFieldClass::GetAddress()
{
	return Class;
}

UEFFieldClass::operator bool() const
{
	return Class != nullptr;
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
	return Class ? GetFName().ToValidString() : "None";
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
	return Field ? GetFName().ToValidString() : "None";
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

void* UEObject::GetVft() const
{
	return *reinterpret_cast<void**>(Object);
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

int32 UEObject::GetPackageIndex() const
{
	return GetOutermost().GetIndex();
}

bool UEObject::HasAnyFlags(EObjectFlags Flags) const
{
	return GetFlags() & Flags;
}

bool UEObject::IsA(EClassCastFlags TypeFlags) const
{
	return (TypeFlags != EClassCastFlags::None ? GetClass().IsType(TypeFlags) : true);
}

bool UEObject::IsA(UEClass Class) const
{
	if (!Class)
		return false;

	for (UEClass Clss = GetClass(); Clss; Clss = Clss.GetSuper().Cast<UEClass>())
	{
		if (Clss == Class)
			return true;
	}

	return false;
}

UEObject UEObject::GetOutermost() const
{
	UEObject Outermost = *this;

	for (UEObject Outer = *this; Outer; Outer = Outer.GetOuter())
	{
		Outermost = Outer;
	}

	return Outermost;
}

std::string UEObject::StringifyObjFlags() const
{
	return *this ? StringifyObjectFlags(GetFlags()) : "NoFlags";
}

std::string UEObject::GetName() const
{
	return Object ? GetFName().ToString() : "None";
}

std::string UEObject::GetNameWithPath() const
{
	return Object ? GetFName().ToRawString() : "None";
}

std::string UEObject::GetValidName() const
{
	return Object ? GetFName().ToValidString() : "None";
}

std::string UEObject::GetCppName() const
{
	static UEClass ActorClass = nullptr;
	static UEClass InterfaceClass = nullptr;

	if (ActorClass == nullptr)
		ActorClass = ObjectArray::FindClassFast("Actor");

	if (InterfaceClass == nullptr)
		InterfaceClass = ObjectArray::FindClassFast("Interface");

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

std::string UEObject::GetFullName(int32& OutNameLength) const
{
	if (*this)
	{
		std::string Temp;

		for (UEObject Outer = GetOuter(); Outer; Outer = Outer.GetOuter())
		{
			Temp = Outer.GetName() + '.' + Temp;
		}

		std::string Name = GetName();
		OutNameLength = Name.size() + 1;

		Name = GetClass().GetName() + ' ' + Temp + Name;

		return Name;
	}

	return "None";
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

std::string UEObject::GetPathName() const
{
	if (*this)
	{
		std::string Temp;

		for (UEObject Outer = GetOuter(); Outer; Outer = Outer.GetOuter())
		{
			Temp = Outer.GetNameWithPath() + "." + Temp;
		}

		std::string Name = GetClass().GetNameWithPath();
		Name += " ";
		Name += Temp;
		Name += GetNameWithPath();

		return Name;
	}

	return "None";
}


UEObject::operator bool() const
{
	// if an object is 0x10000F000 it passes the nullptr check
	return Object != nullptr && reinterpret_cast<void*>(Object + Off::UObject::Class) != nullptr;
}

UEObject::operator uint8* ()
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

	void(*Prd)(void*, void*, void*) = decltype(Prd)(VFT[Off::InSDK::ProcessEvent::PEIndex]);

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

std::vector<std::pair<FName, int64>> UEEnum::GetNameValuePairs() const
{
	struct alignas(0x4) Name08Byte { uint8 Pad[0x08]; };
	struct alignas(0x4) Name16Byte { uint8 Pad[0x10]; };
	struct alignas(0x4) UInt8As64  { uint8 Bytes[0x8]; inline operator int64() const { return Bytes[0]; }; };

	static auto GetNameValuePairsWithIndex = []<typename NameType, typename ValueType>(const TArray<TPair<NameType, ValueType>>& EnumNameValuePairs)
	{
		std::vector<std::pair<FName, int64>> Ret;

		for (int i = 0; i < EnumNameValuePairs.Num(); i++)
		{
			Ret.push_back({ FName(&EnumNameValuePairs[i].First), EnumNameValuePairs[i].Second });
		}

		return Ret;
	};

	static auto GetNameValuePairs = []<typename NameType>(const TArray<NameType>& EnumNameValuePairs)
	{
		std::vector<std::pair<FName, int64>> Ret;

		for (int i = 0; i < EnumNameValuePairs.Num(); i++)
		{
			Ret.push_back({ FName(&EnumNameValuePairs[i]), i });
		}

		return Ret;
	};


	if constexpr (Settings::EngineCore::bCheckEnumNamesInUEnum)
	{
		static auto SetIsNamesOnlyIfDevsTookCrack = [&]<typename NameType>(const TArray<TPair<NameType, UInt8As64>>& EnumNames)
		{
			/* This is a hacky workaround for UEnum::Names which somtimes store the enum-value and sometimes don't. I've seem much of UE, but what drugs did some devs take???? */
			Settings::Internal::bIsEnumNameOnly = EnumNames[0].Second != 0 || EnumNames[1].Second != 1;
		};

		if (Settings::Internal::bUseCasePreservingName)
		{
			SetIsNamesOnlyIfDevsTookCrack(*reinterpret_cast<TArray<TPair<Name16Byte, UInt8As64>>*>(Object + Off::UEnum::Names));
		}
		else
		{
			SetIsNamesOnlyIfDevsTookCrack(*reinterpret_cast<TArray<TPair<Name08Byte, UInt8As64>>*>(Object + Off::UEnum::Names));
		}
	}

	if (Settings::Internal::bIsEnumNameOnly)
	{
		if (Settings::Internal::bUseCasePreservingName)
			return GetNameValuePairs(*reinterpret_cast<TArray<Name16Byte>*>(Object + Off::UEnum::Names));
		
		return GetNameValuePairs(*reinterpret_cast<TArray<Name08Byte>*>(Object + Off::UEnum::Names));
	}
	else
	{
		/* This only applies very very rarely on weir UE4.13 or UE4.14 games where the devs didn't know what they were doing. */
		if (Settings::Internal::bIsSmallEnumValue)
		{
			if (Settings::Internal::bUseCasePreservingName)
				return GetNameValuePairsWithIndex(*reinterpret_cast<TArray<TPair<Name16Byte, UInt8As64>>*>(Object + Off::UEnum::Names));

			return GetNameValuePairsWithIndex(*reinterpret_cast<TArray<TPair<Name08Byte, UInt8As64>>*>(Object + Off::UEnum::Names));
		}

		if (Settings::Internal::bUseCasePreservingName)
			return GetNameValuePairsWithIndex(*reinterpret_cast<TArray<TPair<Name16Byte, int64>>*>(Object + Off::UEnum::Names));
		
		return GetNameValuePairsWithIndex(*reinterpret_cast<TArray<TPair<Name08Byte, int64>>*>(Object + Off::UEnum::Names));
	}
}

std::string UEEnum::GetSingleName(int32 Index) const
{
	return GetNameValuePairs()[Index].first.ToString();
}

std::string UEEnum::GetEnumPrefixedName() const
{
	std::string Temp = GetValidName();

	return Temp[0] == 'E' ? Temp : 'E' + Temp;
}

std::string UEEnum::GetEnumTypeAsStr() const
{
	return "enum class " + GetEnumPrefixedName();
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

int32 UEStruct::GetMinAlignment() const
{
	return *reinterpret_cast<int32*>(Object + Off::UStruct::MinAlignemnt);
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
				Properties.push_back(Field.Cast<UEProperty>());
		}

		return Properties;
	}

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(EClassCastFlags::Property))
			Properties.push_back(Field.Cast<UEProperty>());
	}

	return Properties;
}

std::vector<UEFunction> UEStruct::GetFunctions() const
{
	std::vector<UEFunction> Functions;

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(EClassCastFlags::Function))
			Functions.push_back(Field.Cast<UEFunction>());
	}

	return Functions;
}

UEProperty UEStruct::FindMember(const std::string& MemberName, EClassCastFlags TypeFlags) const
{
	if (!Object)
		return nullptr;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(TypeFlags) && Field.GetName() == MemberName)
			{
				return Field.Cast<UEProperty>();
			}
		}
	}

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(TypeFlags) && Field.GetName() == MemberName)
		{
			return Field.Cast<UEProperty>();
		}
	}

	return nullptr;
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
				return true;
		}
	}
	else
	{
		for (UEField F = GetChild(); F; F = F.GetNext())
		{
			if (F.IsA(EClassCastFlags::Property))
				return true;
		}
	}

	return false;
}

EClassCastFlags UEClass::GetCastFlags() const
{
	return *reinterpret_cast<EClassCastFlags*>(Object + Off::UClass::CastFlags);
}

std::string UEClass::StringifyCastFlags() const
{
	return StringifyClassCastFlags(GetCastFlags());
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
		if (Struct.GetName() != ClassName)
			continue;

		for (UEField Field = Struct.GetChild(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Function) && Field.GetName() == FuncName)
			{
				return Field.Cast<UEFunction>();
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

void* UEFunction::GetExecFunction() const
{
	return *reinterpret_cast<void**>(Object + Off::UFunction::ExecFunction);
}

UEProperty UEFunction::GetReturnProperty() const
{
	for (auto Prop : GetProperties())
	{
		if (Prop.HasPropertyFlags(EPropertyFlags::ReturnParm))
			return Prop;
	}

	return nullptr;
}


std::string UEFunction::StringifyFlags(const char* Seperator)  const
{
	return StringifyFunctionFlags(GetFunctionFlags(), Seperator);
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

EClassCastFlags UEProperty::GetCastFlags() const
{
	auto [Class, FieldClass] = GetClass();

	return Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();
}

UEProperty::operator bool() const
{
	return Base != nullptr && ((Base + Off::UObject::Class) != nullptr || (Base + Off::FField::Class) != nullptr);
}


bool UEProperty::IsA(EClassCastFlags TypeFlags) const
{
	if (GetClass().first)
		return GetClass().first.IsType(TypeFlags);

	return GetClass().second.IsType(TypeFlags);
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
	return *reinterpret_cast<int32*>(Base + Off::Property::ArrayDim);
}

int32 UEProperty::GetSize() const
{
	return *reinterpret_cast<int32*>(Base + Off::Property::ElementSize);
}

int32 UEProperty::GetOffset() const
{
	return *reinterpret_cast<int32*>(Base + Off::Property::Offset_Internal);
}

EPropertyFlags UEProperty::GetPropertyFlags() const
{
	return *reinterpret_cast<EPropertyFlags*>(Base + Off::Property::PropertyFlags);
}

bool UEProperty::HasPropertyFlags(EPropertyFlags PropertyFlag) const
{
	return GetPropertyFlags() & PropertyFlag;
}

bool UEProperty::IsType(EClassCastFlags PossibleTypes) const
{
	return (static_cast<uint64>(GetCastFlags()) & static_cast<uint64>(PossibleTypes)) != 0;
}

std::string UEProperty::GetName() const
{
	return Base ? GetFName().ToString() : "None";
}

std::string UEProperty::GetValidName() const
{
	return Base ? GetFName().ToValidString() : "None";
}

int32 UEProperty::GetAlignment() const
{
	EClassCastFlags TypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	if (TypeFlags & EClassCastFlags::ByteProperty)
	{
		return alignof(uint8); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::UInt16Property)
	{
		return alignof(uint16); // 0x2
	}
	else if (TypeFlags & EClassCastFlags::UInt32Property)
	{
		return alignof(uint32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::UInt64Property)
	{
		return alignof(uint64); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::Int8Property)
	{
		return alignof(int8); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::Int16Property)
	{
		return alignof(int16); // 0x2
	}
	else if (TypeFlags & EClassCastFlags::IntProperty)
	{
		return alignof(int32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::Int64Property)
	{
		return alignof(int64); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::FloatProperty)
	{
		return alignof(float); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::DoubleProperty)
	{
		return alignof(double); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::ClassProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::NameProperty)
	{
		return alignof(int32); // FName is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::StrProperty)
	{
		return alignof(FString); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::TextProperty)
	{
		return alignof(FString); // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::BoolProperty)
	{
		return alignof(bool); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::StructProperty)
	{
		return Cast<UEStructProperty>().GetUnderlayingStruct().GetMinAlignment();
	}
	else if (TypeFlags & EClassCastFlags::ArrayProperty)
	{
		return alignof(TArray<int>); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::DelegateProperty)
	{
		return alignof(int32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::WeakObjectProperty)
	{
		return alignof(int32); // TWeakObjectPtr is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::LazyObjectProperty)
	{
		return alignof(int32); // TLazyObjectPtr is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::SoftClassProperty)
	{
		return alignof(FString); // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::SoftObjectProperty)
	{
		return alignof(FString); // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::ObjectProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::MapProperty)
	{
		return alignof(TArray<int>); // 0x8, TMap contains a TArray
	}
	else if (TypeFlags & EClassCastFlags::SetProperty)
	{
		return alignof(TArray<int>); // 0x8, TSet contains a TArray
	}
	else if (TypeFlags & EClassCastFlags::EnumProperty)
	{
		UEProperty P = Cast<UEEnumProperty>().GetUnderlayingProperty();

		return P ? P.GetAlignment() : 0x1;
	}
	else if (TypeFlags & EClassCastFlags::InterfaceProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::FieldPathProperty)
	{
		return alignof(TArray<int>); // alignof member TArray<FName> and ptr;
	}
	else if (TypeFlags & EClassCastFlags::MulticastSparseDelegateProperty)
	{
		return 0x1; // size in PropertyFixup (alignment isn't greater than size)
	}
	else if (TypeFlags & EClassCastFlags::MulticastInlineDelegateProperty)
	{
		return alignof(TArray<int>);  // alignof member TArray<FName>
	}
	else if (TypeFlags & EClassCastFlags::OptionalProperty)
	{
		UEProperty ValueProperty = Cast<UEOptionalProperty>().GetValueProperty();

		/* If this check is true it means, that there is no bool in this TOptional to check if the value is set */
		if (ValueProperty.GetSize() == GetSize()) [[unlikely]]
			return ValueProperty.GetAlignment();

		return  GetSize() - ValueProperty.GetSize();
	}

	if (Settings::Internal::bUseFProperty)
	{
		static std::unordered_map<void*, int32> UnknownProperties;

		static auto TryFindPropertyRefInOptionalToGetAlignment = [](std::unordered_map<void*, int32>& OutProperties, void* PropertyClass) -> int32
		{
			/* Search for a TOptionalProperty that contains an instance of this property */
			for (UEObject Obj : ObjectArray())
			{
				if (!Obj.IsA(EClassCastFlags::Struct))
					continue;

				for (UEProperty Prop : Obj.Cast<UEStruct>().GetProperties())
				{
					if (!Prop.IsA(EClassCastFlags::OptionalProperty) || Prop.IsA(EClassCastFlags::ObjectPropertyBase))
						continue;

					UEOptionalProperty Optional = Prop.Cast<UEOptionalProperty>();

					/* Safe to use first member, as we're guaranteed to use FProperty */
					if (Optional.GetValueProperty().GetClass().second.GetAddress() == PropertyClass)
						return OutProperties.insert({ PropertyClass, Optional.GetAlignment() }).first->second;
				}
			}

			return OutProperties.insert({ PropertyClass, 0x1 }).first->second;
		};

		auto It = UnknownProperties.find(GetClass().second.GetAddress());

		/* Safe to use first member, as we're guaranteed to use FProperty */
		if (It == UnknownProperties.end())
			return TryFindPropertyRefInOptionalToGetAlignment(UnknownProperties, GetClass().second.GetAddress());

		return It->second;
	}

	return 0x1;
}

std::string UEProperty::GetCppType() const
{
	EClassCastFlags TypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	if (TypeFlags & EClassCastFlags::ByteProperty)
	{
		return Cast<UEByteProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (TypeFlags & EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (TypeFlags & EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (TypeFlags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (TypeFlags & EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (TypeFlags & EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (TypeFlags & EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (TypeFlags & EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (TypeFlags & EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (TypeFlags & EClassCastFlags::ClassProperty)
	{
		return Cast<UEClassProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (TypeFlags & EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (TypeFlags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (TypeFlags & EClassCastFlags::BoolProperty)
	{
		return Cast<UEBoolProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::StructProperty)
	{
		return Cast<UEStructProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::ArrayProperty)
	{
		return Cast<UEArrayProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::WeakObjectProperty)
	{
		return Cast<UEWeakObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::LazyObjectProperty)
	{
		return Cast<UELazyObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::SoftClassProperty)
	{
		return Cast<UESoftClassProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::SoftObjectProperty)
	{
		return Cast<UESoftObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::ObjectProperty)
	{
		return Cast<UEObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::MapProperty)
	{
		return Cast<UEMapProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::SetProperty)
	{
		return Cast<UESetProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::EnumProperty)
	{
		return Cast<UEEnumProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::InterfaceProperty)
	{
		return Cast<UEInterfaceProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::FieldPathProperty)
	{
		return Cast<UEFieldPathProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::DelegateProperty)
	{
		return Cast<UEDelegateProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::OptionalProperty)
	{
		return Cast<UEOptionalProperty>().GetCppType();
	}
	else
	{
		return (GetClass().first ? GetClass().first.GetCppName() : GetClass().second.GetCppName()) + "_";;
	}
}

std::string UEProperty::GetPropClassName() const
{
	return GetClass().first ? GetClass().first.GetName() : GetClass().second.GetName();
}

std::string UEProperty::StringifyFlags() const
{
	return StringifyPropertyFlags(GetPropertyFlags());
}

UEEnum UEByteProperty::GetEnum() const
{
	return UEEnum(*reinterpret_cast<void**>(Base + Off::ByteProperty::Enum));
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
	return reinterpret_cast<Off::BoolProperty::UBoolPropertyBase*>(Base + Off::BoolProperty::Base)->FieldMask;
}

uint8 UEBoolProperty::GetBitIndex() const
{
	uint8 FieldMask = GetFieldMask();

	if (FieldMask != 0xFF)
	{
		if (FieldMask == 0x01) { return 0; }
		if (FieldMask == 0x02) { return 1; }
		if (FieldMask == 0x04) { return 2; }
		if (FieldMask == 0x08) { return 3; }
		if (FieldMask == 0x10) { return 4; }
		if (FieldMask == 0x20) { return 5; }
		if (FieldMask == 0x40) { return 6; }
		if (FieldMask == 0x80) { return 7; }
	}

	return 0xFF;
}

bool UEBoolProperty::IsNativeBool() const
{
	return reinterpret_cast<Off::BoolProperty::UBoolPropertyBase*>(Base + Off::BoolProperty::Base)->FieldMask == 0xFF;
}

std::string UEBoolProperty::GetCppType() const
{
	return IsNativeBool() ? "bool" : "uint8";
}

UEClass UEObjectProperty::GetPropertyClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::ObjectProperty::PropertyClass));
}

std::string UEObjectProperty::GetCppType() const
{
	return std::format("class {}*", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

UEClass UEClassProperty::GetMetaClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::ClassProperty::MetaClass));
}

std::string UEClassProperty::GetCppType() const
{
	return HasPropertyFlags(EPropertyFlags::UObjectWrapper) ? std::format("TSubclassOf<class {}>", GetMetaClass().GetCppName()) : "class UClass*";
}

std::string UEWeakObjectProperty::GetCppType() const
{
	return std::format("TWeakObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

std::string UELazyObjectProperty::GetCppType() const
{
	return std::format("TLazyObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

std::string UESoftObjectProperty::GetCppType() const
{
	return std::format("TSoftObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
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
	return UEStruct(*reinterpret_cast<void**>(Base + Off::StructProperty::Struct));
}

std::string UEStructProperty::GetCppType() const
{
	return std::format("struct {}", GetUnderlayingStruct().GetCppName());
}

UEProperty UEArrayProperty::GetInnerProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::ArrayProperty::Inner));
}

std::string UEArrayProperty::GetCppType() const
{
	return std::format("TArray<{}>", GetInnerProperty().GetCppType());
}

UEFunction UEDelegateProperty::GetSignatureFunction() const
{
	return UEFunction(*reinterpret_cast<void**>(Base + Off::DelegateProperty::SignatureFunction));
}

std::string UEDelegateProperty::GetCppType() const
{
	return "TDeleage<GetCppTypeIsNotImplementedForDelegates>";
}

UEFunction UEMulticastInlineDelegateProperty::GetSignatureFunction() const
{
	// Uses "Off::DelegateProperty::SignatureFunction" on purpose
	return UEFunction(*reinterpret_cast<void**>(Base + Off::DelegateProperty::SignatureFunction));
}

std::string UEMulticastInlineDelegateProperty::GetCppType() const
{
	return "TMulticastInlineDelegate<GetCppTypeIsNotImplementedForDelegates>";
}

UEProperty UEMapProperty::GetKeyProperty() const
{
	return UEProperty(reinterpret_cast<Off::MapProperty::UMapPropertyBase*>(Base + Off::MapProperty::Base)->KeyProperty);
}

UEProperty UEMapProperty::GetValueProperty() const
{
	return UEProperty(reinterpret_cast<Off::MapProperty::UMapPropertyBase*>(Base + Off::MapProperty::Base)->ValueProperty);
}

std::string UEMapProperty::GetCppType() const
{
	return std::format("TMap<{}, {}>", GetKeyProperty().GetCppType(), GetValueProperty().GetCppType());
}

UEProperty UESetProperty::GetElementProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::SetProperty::ElementProp));
}

std::string UESetProperty::GetCppType() const
{
	return std::format("TSet<{}>", GetElementProperty().GetCppType());
}

UEProperty UEEnumProperty::GetUnderlayingProperty() const
{
	return UEProperty(reinterpret_cast<Off::EnumProperty::UEnumPropertyBase*>(Base + Off::EnumProperty::Base)->UnderlayingProperty);
}

UEEnum UEEnumProperty::GetEnum() const
{
	return UEEnum(reinterpret_cast<Off::EnumProperty::UEnumPropertyBase*>(Base + Off::EnumProperty::Base)->Enum);
}

std::string UEEnumProperty::GetCppType() const
{
	if (GetEnum())
		return GetEnum().GetEnumTypeAsStr();

	return GetUnderlayingProperty().GetCppType();
}

UEFFieldClass UEFieldPathProperty::GetFielClass() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Base + Off::FieldPathProperty::FieldClass));
}

std::string UEFieldPathProperty::GetCppType() const
{
	return std::format("TFieldPath<struct {}>", GetFielClass().GetCppName());
}

UEProperty UEOptionalProperty::GetValueProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::OptionalProperty::ValueProperty));
}

std::string UEOptionalProperty::GetCppType() const
{
	return std::format("TOptional<{}>", GetValueProperty().GetCppType());
}