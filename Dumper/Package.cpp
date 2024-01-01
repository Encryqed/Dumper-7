#include <algorithm>

#include "Package.h"
#include "Settings.h"
#include "Generator.h"

std::ofstream Package::DebugAssertionStream;
PackageDependencyManager Package::PackageSorterClasses; // "PackageName_classes.hpp"
PackageDependencyManager Package::PackageSorterStructs; // "PackageName_structs.hpp"
PackageDependencyManager Package::PackageSorterParams; // "PackageName_parameters.hpp"

void PackageDependencyManager::GenerateClassSorted(class Package& Pack, int32 ClassIdx)
{
	auto& [bIsIncluded, Dependencies] = AllDependencies[ClassIdx];

	if (!bIsIncluded)
	{
		bIsIncluded = true;

		for (auto& Dependency : Dependencies)
		{
			GenerateClassSorted(Pack, Dependency);
		}

		Pack.GenerateClass(ObjectArray::GetByIndex<UEClass>(ClassIdx));
	}
}

void PackageDependencyManager::GenerateStructSorted(class Package& Pack, const int32 StructIdx)
{
	auto& [bIsIncluded, Dependencies] = AllDependencies[StructIdx];

	if (!bIsIncluded)
	{
		bIsIncluded = true;

		for (auto& Dependency : Dependencies)
		{
			GenerateStructSorted(Pack, Dependency);
		}

		Pack.GenerateStruct(ObjectArray::GetByIndex<UEStruct>(StructIdx));
	}
}

void PackageDependencyManager::GetIncludesForPackage(
	const int32 Index, 
	EIncludeFileType FileType, 
	std::string& OutRef, 
	bool bCommentOut, 
	PackageDependencyManager* AdditionalDependencies,
	EIncludeFileType AdditionalDepFileType
)
{
	auto& [bIsIncluded, Dependencies] = AllDependencies[Index];

	if (!bIsIncluded)
	{
		bIsIncluded = true;

		if (AdditionalDependencies)
			AdditionalDependencies->GetIncludesForPackage(Index, AdditionalDepFileType, OutRef, bCommentOut);
		
		for (auto& Dependency : Dependencies)
		{
			GetIncludesForPackage(Dependency, FileType, OutRef, bCommentOut, AdditionalDependencies, AdditionalDepFileType);
		}

		std::string PackageName = ObjectArray::GetByIndex(Index).GetName();

		switch (FileType)
		{
		case EIncludeFileType::Struct:
			OutRef += std::format("\n{}#include \"SDK/{}{}_structs.hpp\"", (bCommentOut ? "//" : ""), (Settings::FilePrefix ? Settings::FilePrefix : ""), PackageName);
			break;
		case EIncludeFileType::Class:
			OutRef += std::format("\n{}#include \"SDK/{}{}_classes.hpp\"", (bCommentOut ? "//" : ""), (Settings::FilePrefix ? Settings::FilePrefix : ""), PackageName);
			break;
		case EIncludeFileType::Params:
			OutRef += std::format("\n{}#include \"SDK/{}{}_parameters.hpp\"", (bCommentOut ? "//" : ""), (Settings::FilePrefix ? Settings::FilePrefix : ""), PackageName);
			break;
		default:
			break;
		}
	}
}

void PackageDependencyManager::GetPropertyDependency(UEProperty Prop, std::unordered_set<int32>& Store)
{
	if (Prop.IsA(EClassCastFlags::StructProperty))
	{
		Store.insert(Prop.Cast<UEStructProperty>().GetUnderlayingStruct().GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::EnumProperty))
	{
		if (auto Enum = Prop.Cast<UEEnumProperty>().GetEnum())
			Store.insert(Enum.GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::ByteProperty))
	{
		if (UEObject Enum = Prop.Cast<UEByteProperty>().GetEnum())
		{
			Store.insert(Enum.GetIndex());
		}
	}
	else if (Prop.IsA(EClassCastFlags::ArrayProperty))
	{
		GetPropertyDependency(Prop.Cast<UEArrayProperty>().GetInnerProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::SetProperty))
	{
		GetPropertyDependency(Prop.Cast<UESetProperty>().GetElementProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::MapProperty))
	{
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetKeyProperty(), Store);
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetValueProperty(), Store);
	}
}

void PackageDependencyManager::GetFunctionDependency(UEFunction Func, std::unordered_set<int32>& Store)
{
	for (UEProperty Property : Func.GetProperties())
	{
		PackageDependencyManager::GetPropertyDependency(Property, Store);
	}
}

void Package::InitAssertionStream(const fs::path& GenPath)
{
	if constexpr (Settings::Debug::bGenerateAssertionFile)
	{
		DebugAssertionStream.open(GenPath / "Assertions.h");

		DebugAssertionStream << "#pragma once\n#include\"SDK.hpp\"\n\nusing namespace SDK;\n\n";
	}
}

void Package::CloseAssertionStream()
{
	if constexpr (Settings::Debug::bGenerateAssertionFile)
	{
		DebugAssertionStream.flush();
		DebugAssertionStream.close();
	}
}

int32 Package::GeneratePredefinedMembers(const std::string& ClassName, Types::Struct& Struct, int32 StructSize, int32 SuperSize)
{
	int PrevPropertyEnd = SuperSize;
	int PrevPropertySize = -1;

	auto Predef = Generator::PredefinedMembers.find(ClassName);
	if (Predef != Generator::PredefinedMembers.end())
	{
		Struct.SetCustomAlignment(0x1);

		bool bIsFirstMember = true;
		for (auto& Member : Predef->second)
		{
			if (Member.Offset > PrevPropertyEnd && PrevPropertySize != 0)
			{
				Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, Member.Offset - PrevPropertyEnd, "Fixing Size After Last (Predefined) Property  [ Dumper-7 ]"));
			}

			Struct.AddMember(Types::Member(Member.Type, Member.Name, std::format("(0x{:02X}[0x{:02X}]) NOT AUTO-GENERATED PROPERTY", Member.Offset, Member.Size)));

			PrevPropertySize = Member.Size;
			PrevPropertyEnd = Member.Size > 0 ? Member.Offset + Member.Size : PrevPropertyEnd;
		}

		StructSize = max(StructSize, PrevPropertyEnd);

		if (StructSize > PrevPropertyEnd)
		{
			Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, StructSize - PrevPropertyEnd, "Fixing Size Of Struct [ Dumper-7 ]"));
		}

		return StructSize;
	}

	return 0;
}

Types::Member Package::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return Types::Member("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), Reason);
}

Types::Member Package::GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 BitPadNum = 0;

	return Types::Member("uint8", std::format("BitPad_{:X} : {:X}", BitPadNum++, PadSize), std::move(Reason));
}

void Package::GatherDependencies(const std::vector<int32_t>& PackageMembers)
{
	for (int32_t Index : PackageMembers)
	{
		UEObject Object = ObjectArray::GetByIndex(Index);

		if (!Object)
			continue;

		std::unordered_set<int32> ObjectsToCheck;
		const bool bIsClass = Object.IsA(EClassCastFlags::Class);

		if (Object.IsA(EClassCastFlags::Struct) && !Object.IsA(EClassCastFlags::Function))
		{
			UEStruct Struct = Object.Cast<UEStruct>();

			if (UEStruct Super = Struct.GetSuper())
				ObjectsToCheck.insert(Super.GetIndex());

			for (UEProperty Property : Struct.GetProperties())
			{
				PackageDependencyManager::GetPropertyDependency(Property, ObjectsToCheck);
			}

			for (UEField Field = Struct.GetChild(); Field; Field = Field.GetNext())
			{
				if (Field.IsA(EClassCastFlags::Function))
				{
					PackageDependencyManager::GetFunctionDependency(UEFunction(Field.GetAddress()), ObjectsToCheck);
				}
			}
			for (auto& Idx : ObjectsToCheck)
			{
				UEObject Obj = ObjectArray::GetByIndex(Idx);
			
				UEObject Outermost = Obj.GetOutermost();

				const bool bDependencyIsClass = Obj.IsA(EClassCastFlags::Class);
				const bool bDependencyIsStruct = Obj.IsA(EClassCastFlags::Struct) && !bDependencyIsClass;

				if (PackageObject != Outermost)
				{
					auto& PackageSorter = bDependencyIsClass ? Package::PackageSorterClasses : Package::PackageSorterStructs;

					PackageSorter.AddDependency(PackageObject.GetIndex(), Outermost.GetIndex());
					Package::PackageSorterParams.AddDependency(PackageObject.GetIndex(), Outermost.GetIndex());

					continue;
				}
			
				if (bIsClass && bDependencyIsClass)
				{
					ClassSorter.AddDependency(Object.GetIndex(), Idx);
				}
				else if (!bIsClass && bDependencyIsStruct)
				{
					StructSorter.AddDependency(Object.GetIndex(), Idx);
				}
			}
		}
	}
}

void Package::AddPackage(int32 Idx)
{
	Package::PackageSorterClasses.AddPackage(Idx);
	Package::PackageSorterStructs.AddPackage(Idx);
}

void Package::Process(const std::vector<int32_t>& PackageMembers)
{
	for (int32_t Index : PackageMembers)
	{
		UEObject Object = ObjectArray::GetByIndex(Index);

		if (!Object)
			continue;

		if (Object.IsA(EClassCastFlags::Enum))
		{
			GenerateEnum(Object.Cast<UEEnum>());
		}
		else if (Object.IsA(EClassCastFlags::Class))
		{
			ClassSorter.GenerateClassSorted(*this, Index);
		}
		else if (Object.IsA(EClassCastFlags::Struct) && !Object.IsA(EClassCastFlags::Function))
		{
			StructSorter.GenerateStructSorted(*this, Index);
		}
	}
}

void Package::StaticGenerateMembers(const std::vector<UEProperty>& MemberVector, UEStruct& Super, Types::Struct& Struct, int32 StructSize, int32 SuperSize)
{
	bool bLastPropertyWasBitField = false;

	int PrevPropertyEnd = SuperSize;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	std::string SuperName = Super.GetCppName();

	if (MemberVector.size() == 0 && GeneratePredefinedMembers(SuperName, Struct, StructSize, SuperSize) != 0)
		return; // generated struct based on override, return now
		

	if constexpr (Settings::Debug::bGenerateAssertionFile)
	{
		if constexpr (!Settings::Debug::bLimitAssertionsToEngienPackage)
		{
			if (Super.GetOutermost() == EnginePackage)
			{
				if (!Super.IsA(EClassCastFlags::Function) && !MemberVector.empty())
				{
					Package::DebugAssertionStream << "\n//" << SuperName << "\n";
					Package::DebugAssertionStream << std::format("static_assert(sizeof({}) == 0x{:04X}, \"Class {} has wrong size!\");\n", SuperName, StructSize, SuperName);
				}
			}
		}
	}

	for (auto& Property : MemberVector)
	{
		std::string CppType = Property.GetCppType();
		std::string Name = Property.GetArrayDim() > 1 ? std::format("{}[0x{:X}]", Property.GetValidName(), Property.GetArrayDim()) : Property.GetValidName();

		const int Offset = Property.GetOffset();
		const int Size = (!Property.IsA(EClassCastFlags::StructProperty) ? Property.GetSize() : Property.Cast<UEStructProperty>().GetUnderlayingStruct().GetStructSize()) * Property.GetArrayDim();

		std::string Comment = std::format("0x{:X}(0x{:X})({})", Offset, Size, Property.StringifyFlags());

		if (Offset >= PrevPropertyEnd)
		{
			if (bLastPropertyWasBitField && PrevBoolPropertyBit != 9)
			{
				Struct.AddMember(GenerateBitPadding(Offset, 9 - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]"));
			}
			if (Offset > PrevPropertyEnd)
			{
				Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, Offset - PrevPropertyEnd, "Fixing Size After Last Property  [ Dumper-7 ]"));
			}
		}

		if (Property.IsA(EClassCastFlags::BoolProperty) && !Property.Cast<UEBoolProperty>().IsNativeBool())
		{
			Name += " : 1";

			const uint8 BitIndex = Property.Cast<UEBoolProperty>().GetBitIndex();

			Comment = std::format("Mask: 0x{:X}, PropSize: 0x{:X} {}", Property.Cast<UEBoolProperty>().GetFieldMask(), Size, Comment);

			if (PrevBoolPropertyEnd < Offset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < BitIndex)
			{
				Struct.AddMember(GenerateBitPadding(Offset, BitIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]"));
			}

			PrevBoolPropertyBit = BitIndex + 1;
			PrevBoolPropertyEnd = Offset;

			bLastPropertyWasBitField = true;
		}
		else
		{
			bLastPropertyWasBitField = false;
		}

		Types::Member Member(CppType, Name, Comment);

		PrevPropertyEnd = Offset + Size;

		Struct.AddMember(Member);

		if constexpr (Settings::Debug::bGenerateAssertionFile)
		{
			bool bWriteAssertion = true;

			if constexpr (Settings::Debug::bLimitAssertionsToEngienPackage)
				bWriteAssertion = Super.GetOutermost() == EnginePackage;

			if (bWriteAssertion)
			{
				if (!Super.IsA(EClassCastFlags::Function) && PrevBoolPropertyEnd != Offset)
				{
					size_t SpacePos = Name.find(' ');
					size_t BracePos = Name.find('[');

					Name = Name.substr(0, SpacePos != std::string::npos ? SpacePos : BracePos != std::string::npos ? BracePos : Name.length());

					Package::DebugAssertionStream << std::format("static_assert(offsetof({}, {}) == 0x{:04X}, \"Wrong offset on {}::{}!\");\n", SuperName, Name, Offset, SuperName, Name);
				}
			}
		}
	}

	if (StructSize > PrevPropertyEnd)
	{
		Struct.AddMember(GenerateBytePadding(PrevPropertyEnd, StructSize - PrevPropertyEnd, "Fixing Size Of Struct [ Dumper-7 ]"));
	}
}

Types::Function Package::StaticGenerateFunction(UEFunction& Function, UEStruct& Super)
{
	static int NumUnnamedFunctions = 0;

	std::string ReturnType = "void";
	std::vector<Types::Parameter> Params;
	std::string FuncBody;

	std::string FunctionName = Function.GetValidName();

	if (FunctionName.empty())
		FunctionName = std::format("UnknownFunction_{:04X}", NumUnnamedFunctions++);

	std::vector<std::pair<std::string, bool>> OutPtrParamNames;

	bool bHasRetType = false;
	bool bHasParams = false;

	for (UEProperty Param : Function.GetProperties())
	{
		bHasParams = true;

		bool bIsRef = false;
		bool bIsOut = false;
		bool bIsMoveType = Param.IsA(EClassCastFlags::StructProperty) || Param.IsA(EClassCastFlags::ArrayProperty) || Param.IsA(EClassCastFlags::StrProperty) || Param.IsA(EClassCastFlags::MapProperty) || Param.IsA(EClassCastFlags::SetProperty);
		bool bIsRet = Param.HasPropertyFlags(EPropertyFlags::ReturnParm);

		std::string Type = Param.GetCppType();

		if (!bIsRet && Param.HasPropertyFlags(EPropertyFlags::ReferenceParm))
		{
			Type += "&";
			bIsRef = true;
			bIsOut = true;
		}

		if (!bIsRet && !bIsRef && Param.HasPropertyFlags(EPropertyFlags::OutParm))
		{
			Type += "*";
			bIsOut = true;
			OutPtrParamNames.push_back({ Param.GetValidName(), bIsMoveType });
		}

		if (!bIsRet && !bIsOut && !bIsRef && bIsMoveType)
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

	Types::Function Func(ReturnType, FunctionName, Super.GetCppName(), Params);

	Func.AddComment(Function.GetFullName());
	Func.AddComment("(" + Function.StringifyFlags() + ")");
	Func.AddComment("Parameters:");

	for (UEProperty Param : Function.GetProperties())
	{
		Func.AddComment(std::format("{:{}}{:{}}({})", Param.GetCppType(), 35, Param.GetValidName(), 65, Param.StringifyFlags()));
	}

	FuncBody += "\tstatic class UFunction* Func = nullptr;\n\n\tif (!Func)\n";

	if (Settings::XORString)
	{
		FuncBody += std::format("\t\tFunc = Class->GetFunction({0}(\"{1}\"), {0}(\"{2}\"));\n\n", Settings::XORString, Super.GetName(), Function.GetName());
	}
	else
	{
		FuncBody += std::format("\t\tFunc = Class->GetFunction(\"{}\", \"{}\");\n\n", Super.GetName(), Function.GetName());
	}

	if (bHasParams)
		FuncBody += std::format("\t{}{} Parms{{}};\n", (Settings::ParamNamespaceName ? Settings::ParamNamespaceName + std::string("::") : ""), Function.GetParamStructName());

	for (auto& Param : Func.GetParameters())
	{
		if (!Param.IsParamOutPtr())
			FuncBody += std::format("\n\tParms.{0} = {0};", Param.GetName());
	}

	if (Function.HasFlags(EFunctionFlags::Native))
		FuncBody += "\n\n\tauto Flgs = Func->FunctionFlags;\n\tFunc->FunctionFlags |= 0x400;";

	FuncBody += bHasParams ? "\n\n\tUObject::ProcessEvent(Func, &Parms);\n" : "\n\n\tUObject::ProcessEvent(Func, nullptr);\n";

	if (Function.HasFlags(EFunctionFlags::Native))
		FuncBody += "\n\n\tFunc->FunctionFlags = Flgs;\n";


	for (auto& [Name, bMove] : OutPtrParamNames)
	{
		FuncBody += std::format("\n\tif ({0} != nullptr)\n\t\t*{0} = {1}Parms.{0}{2};\n", Name, bMove ? "std::move(" : "", bMove ? ")" : "");
	}

	if (bHasRetType)
		FuncBody += "\n\treturn Parms.ReturnValue;\n";

	Func.AddBody(FuncBody);

	if (bHasParams)
		Func.SetParamStruct(StaticGenerateStruct(Function, true));

	return Func;
}

Types::Struct Package::StaticGenerateStruct(UEStruct Struct, bool bIsFunction)
{
	std::string StructName = !bIsFunction ? Struct.GetCppName() : Struct.Cast<UEFunction>().GetParamStructName();

	Types::Struct RetStruct(StructName, Struct.GetOutermost().GetName());

	int Size = Struct.GetStructSize();
	int SuperSize = 0;

	auto It = UEStruct::StructSizes.find(Struct.GetIndex());

	if (It != UEStruct::StructSizes.end())
		Size = It->second;

	if (!bIsFunction)
	{
		if (UEStruct Super = Struct.GetSuper())
		{
			RetStruct = Types::Struct(StructName, Struct.GetOutermost().GetName(), false, Super.GetCppName());
			SuperSize = Super.GetStructSize();

			UEObject SuperPackage = Super.GetOutermost();

			auto It = UEStruct::StructSizes.find(Super.GetIndex());

			if (It != UEStruct::StructSizes.end())
				SuperSize = It->second;
		}
	}

	RetStruct.AddComment(std::format("0x{:X} (0x{:X} - 0x{:X})", Size - SuperSize, Size, SuperSize));
	RetStruct.AddComment(Struct.GetFullName());

	std::vector<UEProperty> Properties = Struct.GetProperties();
	
	static int NumProps = 0;
	static int NumFuncs = 0;

	std::sort(Properties.begin(), Properties.end(), [](UEProperty Left, UEProperty Right) -> bool
		{
			if (Left.IsA(EClassCastFlags::BoolProperty) && Right.IsA(EClassCastFlags::BoolProperty))
			{
				if (Left.GetOffset() == Right.GetOffset())
				{
					return Left.Cast<UEBoolProperty>().GetFieldMask() < Right.Cast<UEBoolProperty>().GetFieldMask();
				}
			}

			return Left.GetOffset() < Right.GetOffset();
		});

	StaticGenerateMembers(Properties, Struct, RetStruct, Size, SuperSize);

	return RetStruct;
}

Types::Class Package::StaticGenerateClass(UEClass Class, std::vector<Types::Function>& PackageFunctions)
{
	std::string ClassName = Class.GetCppName();
	std::string RawName = Class.GetName();
	std::string FullName = Class.GetFullName();

	Types::Class RetClass(ClassName, RawName);

	int Size = Class.GetStructSize();
	int SuperSize = 0;

	auto It = UEStruct::StructSizes.find(Class.GetIndex());

	if (It != UEStruct::StructSizes.end())
		Size = It->second;

	if (UEStruct Super = Class.GetSuper())
	{
		RetClass = Types::Class(ClassName, RawName, Super.GetCppName());
		SuperSize = Super.GetStructSize();

		auto It = UEStruct::StructSizes.find(Super.GetIndex());

		if (It != UEStruct::StructSizes.end())
			SuperSize = It->second;
	}

	RetClass.AddComment(std::format("0x{:X} (0x{:X} - 0x{:X})", Size - SuperSize, Size, SuperSize));
	RetClass.AddComment(FullName);

		
	Types::Function StaticClass("class UClass*", "StaticClass", Class.GetCppName(), {}, true);

	StaticClass.AddComment(Class.GetFullName());
	StaticClass.AddComment("(" + Class.StringifyCastFlags() + ")");


	StaticClass.AddBody(
		std::format(
			R"(	static class UClass* Clss = nullptr;

	if (!Clss)
		Clss = UObject::FindClassFast({});

	return Clss;)", Settings::XORString ? std::format("{}(\"{}\")", Settings::XORString, RawName) : std::format("\"{}\"", RawName)));


	Types::Function GetDefault("class " + ClassName + "*", "GetDefaultObj", ClassName, {}, true, true);

	GetDefault.AddComment(Class.GetDefaultObject().GetFullName());
	GetDefault.AddComment("(" + Class.GetDefaultObject().StringifyObjFlags() + ")");

	GetDefault.AddBody(
		std::format(
			R"(	static class {0}* Default = nullptr;

	if (!Default)
		Default = static_cast<{0}*>({0}::StaticClass()->DefaultObject);

	return Default;)", ClassName));

	RetClass.AddFunction(StaticClass);
	RetClass.AddFunction(GetDefault);
	PackageFunctions.push_back(StaticClass);
	PackageFunctions.push_back(GetDefault);

	for (UEField Child = Class.GetChild(); Child; Child = Child.GetNext())
	{
		if (Child.IsA(EClassCastFlags::Function))
		{
			Types::Function Func = StaticGenerateFunction(Child.Cast<UEFunction&>(), Class);

			RetClass.AddFunction(Func);
			PackageFunctions.push_back(Func);
		}
	}

	std::vector<UEProperty> Properties = Class.GetProperties();
	
	std::sort(Properties.begin(), Properties.end(), [](UEProperty Left, UEProperty Right) -> bool
		{
			if (Left.IsA(EClassCastFlags::BoolProperty) && Right.IsA(EClassCastFlags::BoolProperty))
			{
				if (Left.GetOffset() == Right.GetOffset())
				{
					return Left.Cast<UEBoolProperty>().GetFieldMask() < Right.Cast<UEBoolProperty>().GetFieldMask();
				}
			}

			return Left.GetOffset() < Right.GetOffset();
		});

	UEObject PackageObj = Class.GetOutermost();
	StaticGenerateMembers(Properties, Class, RetClass, Size, SuperSize);

	return RetClass;
}

Types::Enum Package::StaticGenerateEnum(UEEnum Enum)
{
	std::string EnumName = Enum.GetEnumTypeAsStr();

	std::vector<std::pair<FName, int64>> NameValue = Enum.GetNameValuePairs();

	Types::Enum Enm(EnumName, "uint8");

	if (UEEnum::BigEnums.find(Enum.GetIndex()) != UEEnum::BigEnums.end())
		Enm = Types::Enum(EnumName, UEEnum::BigEnums[Enum.GetIndex()]);

	for (int i = 0; i < NameValue.size(); i++)
	{
		std::string TooFullOfAName = NameValue[i].first.ToString();

		Enm.AddMember(MakeNameValid(TooFullOfAName.substr(TooFullOfAName.find_last_of("::") + 1)), NameValue[i].second);
	}

	if (EnumName.find("PixelFormat") != -1)
		Enm.FixWindowsConstant("PF_MAX");

	if (EnumName.find("ERaMaterialName") != -1)
		Enm.FixWindowsConstant("TRANSPARENT");

	return Enm;
}

void Package::GenerateFunction(UEFunction& Function, UEStruct& Super)
{
	AllFunctions.push_back(StaticGenerateFunction(Function, Super));
}

void Package::GenerateStruct(UEStruct Struct, bool bIsFunction)
{
	AllStructs.push_back(StaticGenerateStruct(Struct, bIsFunction));
}

void Package::GenerateClass(UEClass Class)
{
	Types::Class RetClass = StaticGenerateClass(Class, AllFunctions);

	AllClasses.push_back(RetClass);
}

void Package::GenerateEnum(UEEnum Enum)
{
	AllEnums.push_back(StaticGenerateEnum(Enum));
}
