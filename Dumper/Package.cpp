#include <algorithm>

#include "Package.h"
#include "Settings.h"
#include "Generator.h"

PackageDependencyManager Package::PackageSorter;

void PackageDependencyManager::GenerateClassSorted(class Package& Pack, int32 ClassIdx)
{
	auto& PackageInfo = AllDependencies[ClassIdx];

	if (!PackageInfo.first)
	{
		PackageInfo.first = true;

		for (auto Dependency : PackageInfo.second)
		{
			GenerateClassSorted(Pack, Dependency.first);
		}

		UEClass Class = ObjectArray::GetByIndex<UEClass>(ClassIdx);

		Pack.GenerateClass(Class);
	}
}

void PackageDependencyManager::GenerateStructSorted(class Package& Pack, int32 StructIdx)
{
	auto& PackageInfo = AllDependencies[StructIdx];

	if (!PackageInfo.first)
	{
		PackageInfo.first = true;

		for (auto Dependency : PackageInfo.second)
		{
			GenerateStructSorted(Pack, Dependency.first);
		}

		UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

		Pack.GenerateStruct(Struct);
	}
}

void PackageDependencyManager::GetIncludesForPackage(int32 PackageIdx, std::string& OutRef, DependencyFlags Info)
{
	auto& PackageInfo = AllDependencies[PackageIdx];
	int& PackageFlags = (int&)PackageInfo.second[PackageIdx];

	std::cout << "Obj: " << ObjectArray::GetByIndex(PackageIdx).GetName() << "\n";
	//std::cout << "Obj: " << ObjectArray::GetByIndex(PackageInfo.first).GetName() << "\n";

	if (!(PackageFlags & AllIncluded) && !(PackageFlags & CurrentlyProcessed))
	{
		PackageFlags &= CurrentlyProcessed;

		for (auto Dependency : PackageInfo.second)
		{
			//GetIncludesForPackage(Dependency.first, OutRef, PackageInfo.second[Dependency.first]);
			GetIncludesForPackage(Dependency.first, OutRef, Dependency.second);
		}

		std::string PackageName = ObjectArray::GetByIndex(PackageIdx).GetName();

		if ((Info & NeedsStructFile) && !(PackageFlags & StructIncluded))
		{
			OutRef += std::format("\n#include \"SDK/{}_structs.hpp\"", PackageName);

			PackageFlags &= StructIncluded;
		}
		if (Info & NeedsClassFile && !(PackageFlags & ClassIncluded))
		{
			if (!(PackageFlags & StructIncluded))
			{
				OutRef += std::format("\n#include \"SDK/{}_structs.hpp\"", PackageName); //Classes need structs
			}

			OutRef += std::format("\n#include \"SDK/{}_classes.hpp\"", PackageName);

			PackageFlags &= AllIncluded;
		}

		PackageFlags &= ~CurrentlyProcessed;
		//OutRef += std::format("\n#include \"SDK/{}_parameters.hpp\"", PackageName);
	}
}

void PackageDependencyManager::GetObjectDependency(UEObject Obj, std::unordered_set<int32>& Store)
{
	if (Obj.IsA(EClassCastFlags::UFunction))
	{
		for (UEField Field = Obj.Cast<UEStruct>().GetChild(); Field; Field = Field.GetNext())
		{
			PackageDependencyManager::GetObjectDependency(Field, Store);
		}
	}
	if (Obj.IsA(EClassCastFlags::UStructProperty))
	{
		Store.insert(Obj.Cast<UEStructProperty>().GetUnderlayingStruct().GetIndex());
	}
	else if (Obj.IsA(EClassCastFlags::UEnumProperty))
	{
		Store.insert(Obj.Cast<UEEnumProperty>().GetEnum().GetIndex());
	}
	else if (Obj.IsA(EClassCastFlags::UByteProperty))
	{
		if (UEObject Enum = Obj.Cast<UEByteProperty>().GetEnum())
		{
			Store.insert(Enum.GetIndex());
		}
	}
	else if (Obj.IsA(EClassCastFlags::UArrayProperty))
	{
		GetObjectDependency(Obj.Cast<UEArrayProperty>().GetInnerProperty(), Store);
	}
	else if (Obj.IsA(EClassCastFlags::USetProperty))
	{
		GetObjectDependency(Obj.Cast<UESetProperty>().GetElementProperty().Cast<UEField>(), Store);
	}
	else if (Obj.IsA(EClassCastFlags::UMapProperty))
	{
		GetObjectDependency(Obj.Cast<UEMapProperty>().GetKeyProperty().Cast<UEField>(), Store);
		GetObjectDependency(Obj.Cast<UEMapProperty>().GetValueProperty().Cast<UEField>(), Store);
	}
}

void Package::GatherDependencies(std::vector<int32_t>& PackageMembers)
{
	for (int32_t Index : PackageMembers)
	{
		std::unordered_set<int32> ObjectsToCheck;

		UEObject Object = ObjectArray::GetByIndex(Index);

		if (!Object)
			continue;

		const bool bIsClass = Object.IsA(EClassCastFlags::UClass);

		if (Object.IsA(EClassCastFlags::UClass) || (Object.IsA(EClassCastFlags::UStruct) && !Object.IsA(EClassCastFlags::UFunction)))
		{
			UEStruct Struct = Object.Cast<UEStruct>();

			if (UEStruct Super = Struct.GetSuper())
			{
				ObjectsToCheck.insert(Super.GetIndex());
			}

			for (UEField Field = Struct.GetChild(); Field; Field = Field.GetNext())
			{
				PackageDependencyManager::GetObjectDependency(Field, ObjectsToCheck);
			}

			for (auto& Idx : ObjectsToCheck)
			{
				UEObject Obj = ObjectArray::GetByIndex(Idx);

				UEObject Outermost = Obj.GetOutermost();

				if (PackageObject != Outermost)
				{
					
					PackageDependencyManager::DependencyFlags Info = Obj.IsA(EClassCastFlags::UClass) ? PackageDependencyManager::NeedsClassFile : Obj.IsA(EClassCastFlags::UStruct) ? PackageDependencyManager::NeedsStructFile : PackageDependencyManager::None;

					if (Info != PackageDependencyManager::None)
					{
						Package::PackageSorter.AddDependency(PackageObject.GetIndex(), Outermost.GetIndex(), Info);
					}

					continue;
				}

				if (bIsClass)
				{
					if (Obj.IsA(EClassCastFlags::UClass))
					{
						ClassSorter.AddDependency(Object.GetIndex(), Idx);
					}
				}
				else
				{
					if (Obj.IsA(EClassCastFlags::UStruct))
					{
						StructSorter.AddDependency(Object.GetIndex(), Idx);
					}
				}
			}
		}
	}
}

void Package::Process(std::vector<int32_t>& PackageMembers)
{
	GatherDependencies(PackageMembers);

	for (int32_t Index : PackageMembers)
	{
		UEObject Object = ObjectArray::GetByIndex(Index);

		if (!Object)
			continue;
		
		if (Object.IsA(EClassCastFlags::UEnum))
		{
			GenerateEnum(Object.Cast<UEEnum&>());
		}
		else if (Object.IsA(EClassCastFlags::UClass))
		{
			ClassSorter.GenerateClassSorted(*this, Index);
		}
		else if (Object.IsA(EClassCastFlags::UStruct) && !Object.IsA(EClassCastFlags::UFunction))
		{
			StructSorter.GenerateStructSorted(*this, Index);
		}
	}
}

void Package::GenerateMembers(std::vector<UEProperty>& MemberVector, UEStruct& Super, Types::Struct& Struct)
{
	int PrevPropertyEnd = Super.GetSuper() ? Super.GetSuper().GetStructSize() : 0;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	const int StructSize = Super.GetStructSize();
	std::string SuperName = Super.GetCppName();

	if (MemberVector.size() == 0)
	{
		if (Generator::PredefinedMembers.find(SuperName) != Generator::PredefinedMembers.end())
		{
			for (auto& Member : Generator::PredefinedMembers[SuperName])
			{
				if (Member.Offset > PrevPropertyEnd)
					Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, Member.Offset - PrevPropertyEnd, "Fixing Size After Last (Predefined) Property  [ Dumper-7 ]"));

				Struct.AddMember(Types::Member(Member.Type, Member.Name, std::format("(0x{:02X}[0x{:02X}]) NOT AUTO-GENERATED PROPERTY", Member.Offset, Member.Size)));

				PrevPropertyEnd = Member.Offset + Member.Size;
			}

			if (StructSize > PrevPropertyEnd)
			{
				Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, StructSize - PrevPropertyEnd, "Fixing Size Of Struct [ Dumper-7 ]"));
			}

			return;
		}
	}

	for (auto& Property : MemberVector)
	{
		std::string CppType = Property.GetCppType();
		std::string Name = Property.GetValidName();

		const int Offset = Property.GetOffset();
		const int Size = Property.GetSize();

		std::string Comment = std::format("0x{:X}(0x{:X})({})", Offset, Size, Property.StringifyFlags());

		if (Offset > PrevPropertyEnd)
		{
			Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, Offset - PrevPropertyEnd, "Fixing Size After Last Property  [ Dumper-7 ]"));
		}

		if (Property.IsA(EClassCastFlags::UBoolProperty) &&  !Property.Cast<UEBoolProperty>().IsNativeBool())
		{
			Name += " : 1";

			const uint8 BitIndex = Property.Cast<UEBoolProperty>().GetBitIndex();

			Comment = std::format("Mask : 0x{:X} {}", Property.Cast<UEBoolProperty>().GetFieldMask(), Comment);

			if (PrevBoolPropertyEnd < Offset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < BitIndex)
			{
				Types::Member mem = GenerateBitPadding(Offset, BitIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");
				Struct.AddMember(mem);
			}

			PrevBoolPropertyBit = BitIndex + 1;
			PrevBoolPropertyEnd = Offset;
		}

		Types::Member Member(CppType, Name, Comment);

		PrevPropertyEnd = Offset + Size;

		Struct.AddMember(Member);
	}

	if (StructSize > PrevPropertyEnd)
		Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, StructSize - PrevPropertyEnd, "Fixing Size Of Struct [ Dumper-7 ]"));
}

Types::Function Package::GenerateFunction(UEFunction& Function, UEStruct& Super)
{
	std::string ReturnType = "void";
	std::vector<Types::Parameter> Params;
	std::string FuncBody;

	std::vector<std::string> OutPtrParamNames;

	bool bHasRetType = false;
	
	for (UEProperty Param = Function.GetChild().Cast<UEProperty>(); Param; Param = Param.GetNext().Cast<UEProperty>())
	{
		bool bIsRef = false;
		bool bIsOut = false;
		bool bIsRet = Param.HasPropertyFlags(EPropertyFlags::ReturnParm);

		std::string Type = Param.GetCppType();

		if (Param.HasPropertyFlags(EPropertyFlags::ReferenceParm))
		{
			Type += "&";
			bIsRef = true;
			bIsOut = true;
		}
		if (Param.HasPropertyFlags(EPropertyFlags::OutParm) && !bIsRef && !Param.HasPropertyFlags(EPropertyFlags::ReturnParm))
		{
			Type += "*";
			bIsOut = true;
			OutPtrParamNames.push_back(Param.GetValidName());
		}

		if (!bIsRet && !bIsOut && !bIsRef && (Param.IsA(EClassCastFlags::UStructProperty) || Param.IsA(EClassCastFlags::UArrayProperty) || Param.IsA(EClassCastFlags::UStrProperty)))
		{
			Type += "&";
			
			Type = "const " + Type;
		}

		if (bIsRet)
		{
			ReturnType = Type;
			bHasRetType = true;
		}
		else
		{
			Params.push_back(Types::Parameter(Type, Param.GetValidName(), bIsOut && !bIsRef));
		}
	}
	
	Types::Function Func(ReturnType, Function.GetValidName(), Super.GetCppName(), Params);

	Func.AddComment(Function.GetFullName());
	Func.AddComment("(" + Function.StringifyFlags() + ")");
	Func.AddComment("Parameters:");

	for (UEProperty Param = Function.GetChild().Cast<UEProperty>(); Param; Param = Param.GetNext().Cast<UEProperty>())
	{
		Func.AddComment(std::format("{:{}}{:{}}({})", Param.GetCppType(), 35, Param.GetValidName(), 65, Param.StringifyFlags()));
	}

	if (Settings::bShouldXorStrings)
	{
		FuncBody += std::format("\tstatic auto Func = Class->GetFunction({0}(\"{1}\"), {0}(\"{2}\"));\n\n", Settings::XORString, Super.GetName(), Function.GetName());
	}
	else
	{
		FuncBody += std::format("\tstatic auto Func = Class->GetFunction(\"{}\", \"{}\");\n\n", Super.GetName(), Function.GetName());
	}

	FuncBody += std::format("\t{}{} Parms;\n", (Settings::bUseNamespaceForParams ? Settings::ParamNamespaceName + std::string("::") : ""), Function.GetParamStructName());

	for (auto& Param : Func.GetParameters())
	{
		if(!Param.IsParamOutPtr())
			FuncBody += std::format("\tParms.{0} = {0};\n", Param.GetName());
	}

	if (Function.HasFlags(EFunctionFlags::Native))
		FuncBody += "\n\tauto Flags = Func->FunctionFlags;\n\tFunc->FunctionFlags |= 0x400;\n\n";

	FuncBody += "\n\tUObject::ProcessEvent(Func, &Parms);";

    if (Function.HasFlags(EFunctionFlags::Native))
        FuncBody += "\n\n\tFunc->FunctionFlags = Flags;\n\n";


	for (auto& Name : OutPtrParamNames)
	{
		FuncBody += std::format("\n\tif ({0} != nullptr)\n\t\t*{0} = Parms.{0};\n", Name);
	}

	if (bHasRetType)
		FuncBody += "\n\treturn Parms.ReturnValue;\n";

	Func.AddBody(FuncBody);
	Func.SetParamStruct(GenerateStruct(Function, true));

	AllFunctions.push_back(Func);

	return Func;
}

Types::Struct Package::GenerateStruct(UEStruct& Struct, bool bIsFunction)
{
	std::string StructName = !bIsFunction ? Struct.GetCppName() : Struct.Cast<UEFunction>().GetParamStructName();

	Types::Struct RetStruct(StructName);

	int Size = Struct.GetStructSize();
	int SuperSize = 0;

	if (!bIsFunction)
	{
		if (UEStruct Super = Struct.GetSuper())
		{
			RetStruct = Types::Struct(StructName, false, Super.GetCppName());
			SuperSize = Super.GetStructSize();

			UEObject SuperPackage = Super.GetOutermost();

			//if (PackageObject != SuperPackage)
			//	PackageSorter.AddDependency(PackageObject.GetIndex(), SuperPackage.GetIndex());
		}
	}

	RetStruct.AddComment(std::format("0x{:X} (0x{:X} - 0x{:X})", Size - SuperSize, Size, SuperSize));
	RetStruct.AddComment(Struct.GetFullName());

	std::vector<UEProperty> Properties;

	static int NumProps = 0;
	static int NumFuncs = 0;

	for (UEField Child = Struct.GetChild(); Child; Child = Child.GetNext())
	{
		if (Child.IsA(EClassCastFlags::UProperty))
		{
			Properties.push_back(Child.Cast<UEProperty>());
		}
	}

	std::sort(Properties.begin(), Properties.end(), [](UEProperty Left, UEProperty Right) -> bool
		{
			if (Left.IsA(EClassCastFlags::UBoolProperty) && Right.IsA(EClassCastFlags::UBoolProperty))
			{
				if (Left.GetOffset() == Right.GetOffset())
				{
					return Left.Cast<UEBoolProperty>().GetFieldMask() < Right.Cast<UEBoolProperty>().GetFieldMask();
				}
			}

			return Left.GetOffset() < Right.GetOffset();
		});

	GenerateMembers(Properties, Struct, RetStruct);

	if (!bIsFunction)
		AllStructs.push_back(RetStruct);

	return RetStruct;
}

Types::Class Package::GenerateClass(UEClass& Class)
{
	std::string ClassName = Class.GetCppName();

	Types::Class RetClass(ClassName, Class.GetName());

	int Size = Class.GetStructSize();
	int SuperSize = 0;

	if (UEStruct Super = Class.GetSuper())
	{
		RetClass = Types::Class(ClassName, Class.GetName(), Super.GetCppName());
		SuperSize = Super.GetStructSize();

		UEObject SuperPackage = Super.GetOutermost();

		//if (PackageObject != SuperPackage)
		//	PackageSorter.AddDependency(PackageObject.GetIndex(), SuperPackage.GetIndex());
	}

	RetClass.AddComment(std::format("0x{:X} (0x{:X} - 0x{:X})", Size - SuperSize, Size, SuperSize));
	RetClass.AddComment(Class.GetFullName());

	std::vector<UEProperty> Properties;

	static int NumProps = 0;
	static int NumFuncs = 0;

	for (UEField Child = Class.GetChild(); Child; Child = Child.GetNext())
	{
		if (Child.IsA(EClassCastFlags::UProperty))
		{
			Properties.push_back(Child.Cast<UEProperty>());
		}
		else if (Child.IsA(EClassCastFlags::UFunction))
		{
			RetClass.AddFunction(GenerateFunction(Child.Cast<UEFunction&>(), Class));
		}
	}

	std::sort(Properties.begin(), Properties.end(), [](UEProperty Left, UEProperty Right) -> bool
		{
			if (Left.IsA(EClassCastFlags::UBoolProperty) && Right.IsA(EClassCastFlags::UBoolProperty))
			{
				if (Left.GetOffset() == Right.GetOffset())
				{
					return Left.Cast<UEBoolProperty>().GetFieldMask() < Right.Cast<UEBoolProperty>().GetFieldMask();
				}
			}

			return Left.GetOffset() < Right.GetOffset();
		});

	UEObject PackageObj = Class.GetOutermost();
	GenerateMembers(Properties, Class, RetClass);

	AllClasses.push_back(RetClass);

	return RetClass;
}

Types::Enum Package::GenerateEnum(UEEnum& Enum)
{
	std::string EnumName = Enum.GetEnumTypeAsStr();

	auto& NameValue = Enum.GetNameValuePairs();

	Types::Enum Enm(EnumName, "uint8");
	
	if (UEEnum::BigEnums.find(Enum.GetIndex()) != UEEnum::BigEnums.end())
		Enm = Types::Enum(EnumName, UEEnum::BigEnums[Enum.GetIndex()]);
	

	for (int i = 0; i < NameValue.Num(); i++)
	{
		std::string TooFullOfAName = NameValue[i].First.ToString();

		Enm.AddMember(TooFullOfAName.substr(TooFullOfAName.find_last_of(":") + 1), NameValue[i].Second);
	}

	if (EnumName.find("PixelFormat") != -1)
		Enm.FixPFMAX();
	

	AllEnums.push_back(Enm);

	return Enm;
}

Types::Member Package::GenerateBytePadding(int32 Offset, int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return Types::Member("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), Reason);
}

Types::Member Package::GenerateBitPadding(int32 Offset, int32 PadSize, std::string&& Reason)
{
	return Types::Member("uint8", std::format(": {:X}", PadSize), Reason);
}
