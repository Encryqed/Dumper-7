#include "Generators/DumpspaceGenerator.h"

std::string DumpspaceGenerator::GetStructPrefixedName(const StructWrapper& Struct)
{
	if (Struct.IsFunction())
		return Struct.GetUnrealStruct().GetOuter().GetValidName() + "_" + Struct.GetName();

	auto [ValidName, bIsUnique] = Struct.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	/* Package::FStructName */
	return PackageManager::GetName(Struct.GetUnrealStruct().GetPackageIndex()) + "::" + ValidName;
}

std::string DumpspaceGenerator::GetEnumPrefixedName(const EnumWrapper& Enum)
{
	auto [ValidName, bIsUnique] = Enum.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	/* Package::ESomeEnum */
	return PackageManager::GetName(Enum.GetUnrealEnum().GetPackageIndex()) + "::" + ValidName;
}

std::string DumpspaceGenerator::EnumSizeToType(const int32 Size)
{
	static constexpr std::array<const char*, 8> UnderlayingTypesBySize = {
		"uint8",
		"uint16",
		"InvalidEnumSize",
		"uint32",
		"InvalidEnumSize",
		"InvalidEnumSize",
		"InvalidEnumSize",
		"uint64"
	};

	return Size <= 0x8 ? UnderlayingTypesBySize[static_cast<size_t>(Size) - 1] : "uint8";
}

DSGen::EType DumpspaceGenerator::GetMemberEType(const PropertyWrapper& Property)
{
	/* Predefined members are currently not supported by DumpspaceGenerator */
	if (!Property.IsUnrealProperty())
		return DSGen::ET_Default;

	return GetMemberEType(Property.GetUnrealProperty());
}

DSGen::EType DumpspaceGenerator::GetMemberEType(UEProperty Prop)
{
	if (Prop.IsA(EClassCastFlags::EnumProperty))
	{
		return DSGen::ET_Enum;
	}
	else if (Prop.IsA(EClassCastFlags::ByteProperty))
	{
		if (Prop.Cast<UEByteProperty>().GetEnum())
			return DSGen::ET_Enum;
	}
	//else if (Prop.IsA(EClassCastFlags::ClassProperty))
	//{
	//	/* Check if this is a UClass*, not TSubclassof<UObject> */
	//	if (!Prop.Cast<UEClassProperty>().HasPropertyFlags(EPropertyFlags::UObjectWrapper))
	//		return DSGen::ET_Class; 
	//}
	else if (Prop.IsA(EClassCastFlags::ObjectProperty))
	{
		return DSGen::ET_Class;
	}
	else if (Prop.IsA(EClassCastFlags::StructProperty))
	{
		return DSGen::ET_Struct;
	}
	else if (Prop.IsType(EClassCastFlags::ArrayProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty))
	{
		return DSGen::ET_Class;
	}

	return DSGen::ET_Default;
}

std::string DumpspaceGenerator::GetMemberTypeStr(UEProperty Property, std::string& OutExtendedType, std::vector<DSGen::MemberType>& OutSubtypes)
{
	UEProperty Member = Property;

	auto [Class, FieldClass] = Member.GetClass();

	EClassCastFlags Flags = Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();

	if (Flags & EClassCastFlags::ByteProperty)
	{
		if (UEEnum Enum = Member.Cast<UEByteProperty>().GetEnum())
			return GetEnumPrefixedName(Enum);

		return "uint8";
	}
	else if (Flags & EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (Flags & EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (Flags & EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (Flags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (Flags & EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (Flags & EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (Flags & EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (Flags & EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (Flags & EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (Flags & EClassCastFlags::ClassProperty)
	{
		if (Member.HasPropertyFlags(EPropertyFlags::UObjectWrapper))
		{
			OutSubtypes.emplace_back(GetMemberType(Member.Cast<UEClassProperty>().GetMetaClass()));

			return "TSubclassOf";
		}

		OutExtendedType = "*";

		return "UClass";
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return "FName";
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return "FString";
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return "FText";
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return Member.Cast<UEBoolProperty>().IsNativeBool() ? "bool" : "uint8";
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		const StructWrapper& UnderlayingStruct = Member.Cast<UEStructProperty>().GetUnderlayingStruct();

		return GetStructPrefixedName(UnderlayingStruct);
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		OutSubtypes.push_back(GetMemberType(Member.Cast<UEArrayProperty>().GetInnerProperty()));

		return "TArray";
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEWeakObjectProperty>().GetPropertyClass()) 
		{
			OutSubtypes.push_back(GetMemberType(PropertyClass));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Class, "UObject"));
		}

		return "TWeakObjectPtr";
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UELazyObjectProperty>().GetPropertyClass())
		{
			OutSubtypes.push_back(GetMemberType(PropertyClass));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Class, "UObject"));
		}

		return "TLazyObjectPtr";
	}
	else if (Flags & EClassCastFlags::SoftClassProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftClassProperty>().GetPropertyClass())
		{
			OutSubtypes.push_back(GetMemberType(PropertyClass));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Class, "UClass"));
		}

		return "TSoftClassPtr";
	}
	else if (Flags & EClassCastFlags::SoftObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftObjectProperty>().GetPropertyClass())
		{
			OutSubtypes.push_back(GetMemberType(PropertyClass));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Class, "UObject"));
		}

		return "TSoftObjectPtr";
	}
	else if (Flags & EClassCastFlags::ObjectProperty)
	{
		OutExtendedType = "*";

		if (UEClass PropertyClass = Member.Cast<UEObjectProperty>().GetPropertyClass())
			return GetStructPrefixedName(PropertyClass);
		
		return "UObject";
	}
	else if (Settings::EngineCore::bEnableEncryptedObjectPropertySupport && Flags & EClassCastFlags::ObjectPropertyBase && Member.GetSize() == 0x10)
	{
		if (UEClass PropertyClass = Member.Cast<UEObjectProperty>().GetPropertyClass())
			return std::format("TEncryptedObjPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TEncryptedObjPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		UEMapProperty MemberAsMapProperty = Member.Cast<UEMapProperty>();

		OutSubtypes.emplace_back(GetMemberType(Member.Cast<UEMapProperty>().GetKeyProperty()));
		OutSubtypes.emplace_back(GetMemberType(Member.Cast<UEMapProperty>().GetValueProperty()));

		return "TMap";
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		OutSubtypes.emplace_back(GetMemberType(Member.Cast<UESetProperty>().GetElementProperty()));

		return "TSet";
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		if (UEEnum Enum = Member.Cast<UEEnumProperty>().GetEnum())
			return GetEnumPrefixedName(Enum);

		return "NamelessEnumIGuessIdkWhatToPutHereWithRegardsTheGuyFromDumper7";
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEInterfaceProperty>().GetPropertyClass())
		{
			OutSubtypes.push_back(GetMemberType(PropertyClass));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Class, "IInterface"));
		}

		return "TScriptInterface";
	}
	else if (Flags & EClassCastFlags::FieldPathProperty)
	{

		if (Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty)
		{
			OutExtendedType = "*";

			if (UEClass PropertyClass = Member.Cast<UEObjectProperty>().GetPropertyClass())
				return GetStructPrefixedName(PropertyClass);

			return "UObject";
		}

		if (UEFFieldClass PropertyClass = Member.Cast<UEFieldPathProperty>().GetFieldClass())
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Struct, PropertyClass.GetCppName()));
		}
		else
		{
			OutSubtypes.push_back(ManualCreateMemberType(DSGen::ET_Struct, "FField"));
		}

		return "TFieldPath";
	}
	else if (Flags & EClassCastFlags::OptionalProperty)
	{
		UEProperty ValueProperty = Member.Cast<UEOptionalProperty>().GetValueProperty();

		OutSubtypes.push_back(GetMemberType(ValueProperty));

		return "TOptional";
	}
	else
	{
		/* When changing this also change 'GetUnknownProperties()' */
		return (Class ? Class.GetCppName() : FieldClass.GetCppName()) + "_";
	}
}

DSGen::MemberType DumpspaceGenerator::GetMemberType(const StructWrapper& Struct)
{
	DSGen::MemberType Type;
	Type.type = Struct.IsClass() ? DSGen::ET_Class : DSGen::ET_Struct;
	Type.typeName = GetStructPrefixedName(Struct);
	Type.extendedType = Struct.IsClass() ? "*" : "";
	Type.reference = false;

	return Type;
}

DSGen::MemberType DumpspaceGenerator::GetMemberType(const PropertyWrapper& Property, bool bIsReference)
{
	DSGen::MemberType Type;

	if (!Property.IsUnrealProperty())
	{
		Type.typeName = "Unsupported_Predefined_Member";
		return Type;
	}

	Type.reference = bIsReference;
	Type.type = GetMemberEType(Property);
	Type.typeName = GetMemberTypeStr(Property.GetUnrealProperty(), Type.extendedType, Type.subTypes);

	return Type;
}

DSGen::MemberType DumpspaceGenerator::GetMemberType(UEProperty Property, bool bIsReference)
{
	DSGen::MemberType Type;

	Type.reference = bIsReference;
	Type.type = GetMemberEType(Property);
	Type.typeName = GetMemberTypeStr(Property, Type.extendedType, Type.subTypes);

	return Type;
}

DSGen::MemberType DumpspaceGenerator::ManualCreateMemberType(DSGen::EType Type, const std::string& TypeName, const std::string& ExtendedType)
{
	return DSGen::createMemberType(Type, TypeName, ExtendedType);
}

void DumpspaceGenerator::AddMemberToStruct(DSGen::ClassHolder& Struct, const PropertyWrapper& Property)
{
	DSGen::MemberDefinition Member;
	Member.memberType = GetMemberType(Property);
	Member.bitOffset = Property.IsBitField() ? Property.GetBitIndex() : -1;
	Member.offset = Property.GetOffset();
	Member.size = Property.GetSize() * Property.GetArrayDim();
	Member.memberName = Property.GetName();
	Member.arrayDim = Property.GetArrayDim();

	Struct.members.push_back(std::move(Member));
}

void DumpspaceGenerator::RecursiveGetSuperClasses(const StructWrapper& Struct, std::vector<std::string>& OutSupers)
{
	const StructWrapper& Super = Struct.GetSuper();

	OutSupers.push_back(Struct.GetUniqueName().first);

	if (Super.IsValid())
		RecursiveGetSuperClasses(Super, OutSupers);
}

std::vector<std::string> DumpspaceGenerator::GetSuperClasses(const StructWrapper& Struct)
{
	std::vector<std::string> RetSuperNames;

	const StructWrapper& Super = Struct.GetSuper();

	if (Super.IsValid())
		RecursiveGetSuperClasses(Super, RetSuperNames);

	return RetSuperNames;
}

DSGen::ClassHolder DumpspaceGenerator::GenerateStruct(const StructWrapper& Struct)
{
	DSGen::ClassHolder StructOrClass;
	StructOrClass.className = GetStructPrefixedName(Struct);
	StructOrClass.classSize = Struct.GetSize();
	StructOrClass.classType = Struct.IsClass() ? DSGen::ET_Class : DSGen::ET_Struct;
	StructOrClass.interitedTypes = GetSuperClasses(Struct);

	MemberManager Members = Struct.GetMembers();

	for (const PropertyWrapper& Wrapper : Members.IterateMembers())
		AddMemberToStruct(StructOrClass, Wrapper);

	if (!Struct.IsClass())
		return StructOrClass;

	for (const FunctionWrapper& Wrapper : Members.IterateFunctions())
		StructOrClass.functions.push_back(GenearateFunction(Wrapper));

	return StructOrClass;
}

DSGen::EnumHolder DumpspaceGenerator::GenerateEnum(const EnumWrapper& Enum)
{
	DSGen::EnumHolder Enumerator;
	Enumerator.enumName = GetEnumPrefixedName(Enum);
	Enumerator.enumType = EnumSizeToType(Enum.GetUnderlyingTypeSize());

	Enumerator.enumMembers.reserve(Enum.GetNumMembers());
		
	for (const EnumCollisionInfo& Info : Enum.GetMembers())
		Enumerator.enumMembers.emplace_back(Info.GetUniqueName(), Info.GetValue());

	return Enumerator;
}

DSGen::FunctionHolder DumpspaceGenerator::GenearateFunction(const FunctionWrapper& Function)
{
	DSGen::FunctionHolder RetFunc;

	StructWrapper FuncAsStruct = Function.AsStruct();
	MemberManager FuncParams = FuncAsStruct.GetMembers();

	RetFunc.functionName = Function.GetName();
	RetFunc.functionOffset = Function.GetExecFuncOffset();
	RetFunc.functionFlags = Function.StringifyFlags("|");
	RetFunc.returnType = ManualCreateMemberType(DSGen::ET_Default, "void");

	for (const PropertyWrapper& Param : FuncParams.IterateMembers())
	{
		if (!Param.HasPropertyFlags(EPropertyFlags::Parm))
			continue;

		if (Param.HasPropertyFlags(EPropertyFlags::ReturnParm))
		{
			RetFunc.returnType = GetMemberType(Param);
			continue;
		}

		RetFunc.functionParams.emplace_back(GetMemberType(Param), Param.GetName());
	}

	return RetFunc;
}

void DumpspaceGenerator::GeneratedStaticOffsets()
{
	DSGen::addOffset("Dumper", 7);

	DSGen::addOffset("OFFSET_GOBJECTS", Off::InSDK::ObjArray::GObjects);
	DSGen::addOffset(Off::InSDK::Name::bIsUsingAppendStringOverToString ? "OFFSET_APPENDSTRING" : "OFFSET_TOSTRING", Off::InSDK::Name::AppendNameToString);
	DSGen::addOffset("OFFSET_GNAMES", Off::InSDK::NameArray::GNames);
	DSGen::addOffset("OFFSET_GWORLD", Off::InSDK::World::GWorld);
	DSGen::addOffset("OFFSET_PROCESSEVENT", Off::InSDK::ProcessEvent::PEOffset);
	DSGen::addOffset("INDEX_PROCESSEVENT", Off::InSDK::ProcessEvent::PEIndex);
}

void DumpspaceGenerator::Generate()
{
	/* Set the output directory of DSGen to "...GenerationPath/GameVersion-GameName/Dumespace" */
	DSGen::setDirectory(MainFolder);

	/* Add offsets for GObjects, GNames, GWorld, AppendString, PrcessEvent and ProcessEventIndex*/
	GeneratedStaticOffsets();

	// Generates all packages and writes them to files
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		/*
		* Generate classes/structs/enums/functions directly into the respective files
		*
		* Note: Some filestreams aren't opened but passed as parameters anyway because the function demands it, they are not used if they are closed
		*/
		for (int32 EnumIdx : Package.GetEnums())
		{
			DSGen::EnumHolder Enum = GenerateEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx));
			DSGen::bakeEnum(Enum);
		}

		DependencyManager::OnVisitCallbackType GenerateClassOrStructCallback = [&](int32 Index) -> void
		{
			DSGen::ClassHolder StructOrClass = GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index));
			DSGen::bakeStructOrClass(StructOrClass);
		};

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			Structs.VisitAllNodesWithCallback(GenerateClassOrStructCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();

			Classes.VisitAllNodesWithCallback(GenerateClassOrStructCallback);
		}
	}

	DSGen::dump();
}
