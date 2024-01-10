#include "CppGenerator.h"
#include "ObjectArray.h"
#include "MemberWrappers.h"
#include "MemberManager.h"
#include "PackageManager.h"


std::string CppGenerator::MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment)
{
	return std::format("\t{:{}}{:{}} // {}\n", Type, 45, Name + ";", 50, std::move(Comment));
}

std::string CppGenerator::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return MakeMemberString("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 BitPadNum = 0;

	return MakeMemberString("uint8", std::format("BitPad_{:X} : {:X}", BitPadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize)
{
	constexpr uint64 EstimatedCharactersPerLine = 0x80;
	constexpr uint64 NumBitsInBytePlusOne = 0x9;

	std::string OutMembers;
	OutMembers.reserve(Members.GetNumMembers() * EstimatedCharactersPerLine);

	bool bLastPropertyWasBitField = false;

	int PrevPropertyEnd = SuperSize;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		int32 MemberOffset = Member.GetOffset();
		int32 MemberSize = Member.GetSize();

		std::string Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, StringifyPropertyFlags(Member.GetPropertyFlags()));

		if (MemberOffset >= PrevPropertyEnd && bLastPropertyWasBitField && PrevBoolPropertyBit != NumBitsInBytePlusOne)
			OutMembers += GenerateBitPadding(MemberOffset, NumBitsInBytePlusOne - PrevBoolPropertyBit, "Fixing Bit-Field Size [ Dumper-7 ]");

		if (MemberOffset > PrevPropertyEnd)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, MemberOffset - PrevPropertyEnd, "Fixing Size After Last Property [ Dumper-7 ]");

		const bool bIsBitField = Member.IsBitField();

		if (bIsBitField)
		{
			uint8 BitFieldIndex = Member.GetBitIndex();

			std::string BitfieldInfoComment = std::format("Mask: 0x{:02X}, PropSize: 0x{:04X} ({})", Member.GetFieldMask(), MemberSize, StringifyPropertyFlags(Member.GetPropertyFlags()));
			Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, BitfieldInfoComment);

			if (PrevBoolPropertyEnd < MemberOffset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < BitFieldIndex)
				OutMembers += GenerateBitPadding(MemberOffset, BitFieldIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

			PrevBoolPropertyBit = BitFieldIndex + 1;
			PrevBoolPropertyEnd = MemberOffset;
		}

		bLastPropertyWasBitField = bIsBitField;

		PrevPropertyEnd = MemberOffset + MemberSize;

		std::string MemberName = Member.GetName();

		if (Member.GetArrayDim() > 1)
		{
			MemberName += std::format("[0x{:X}]", Member.GetArrayDim());
		}
		else if (bIsBitField)
		{
			MemberName += " : 1";
		}

		OutMembers += MakeMemberString(GetMemberTypeString(Member), MemberName, std::move(Comment));
	}

	return OutMembers;
}

std::string CppGenerator::GenerateFunctionInHeader(const MemberManager& Members)
{
	std::string AllFuntionsText;

	bool bIsFirstIteration = true;
	bool bDidSwitch = false;
	bool bWasLastFuncStatic = false;
	bool bWasLastFuncInline = false;
	bool bWaslastFuncConst = false;

	for (const FunctionWrapper& Func : Members.IterateFunctions())
	{
		if (bWasLastFuncInline != Func.HasInlineBody() && !bIsFirstIteration)
		{
			AllFuntionsText += "\npublic:\n";
			bDidSwitch = true;
		}

		if ((bWasLastFuncStatic != Func.IsStatic() || bWaslastFuncConst != Func.IsConst()) && !bIsFirstIteration && !bDidSwitch)
			AllFuntionsText += '\n';

		bWasLastFuncStatic = Func.IsStatic();
		bWasLastFuncInline = Func.HasInlineBody();
		bWaslastFuncConst = Func.IsConst();
		bIsFirstIteration = false;
		bDidSwitch = false;

		if (Func.IsPredefined())
		{
			AllFuntionsText += std::format("\t{}{} {}{}", Func.IsStatic() ? "static " : "", Func.GetPredefFuncReturnType(), Func.GetPredefFuncNameWithParams(), Func.IsConst() ? " const" : "");

			AllFuntionsText += (Func.HasInlineBody() ? ("\n\t" + Func.GetPredefFunctionInlineBody()) : ";") + "\n";
			continue;
		}

		MemberManager FuncParams = Func.GetMembers();

		std::string RetType = "void";
		std::string Params;

		bool bIsFirstParam = true;

		for (const PropertyWrapper& Param : FuncParams.IterateMembers())
		{
			std::string Type = GetMemberTypeString(Param);

			if (Param.IsReturnParam())
			{
				RetType = GetMemberTypeString(Param);
				continue;
			}

			bool bIsRef = false;
			bool bIsOut = false;
			bool bIsConst = Param.HasPropertyFlags(EPropertyFlags::ConstParm);
			bool bIsMoveType = Param.IsType(EClassCastFlags::StructProperty | EClassCastFlags::ArrayProperty | EClassCastFlags::StrProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty);
		
			if (Param.HasPropertyFlags(EPropertyFlags::ReferenceParm))
			{
				Type += "&";
				bIsRef = true;
				bIsOut = true;
			}

			if (!bIsRef && Param.HasPropertyFlags(EPropertyFlags::OutParm))
			{
				Type += "*";
				bIsOut = true;
			}

			if (bIsConst)
				Type = "const " + Type;

			if (!bIsOut && !bIsRef && bIsMoveType)
			{
				Type += "&";

				if (!bIsConst)
					Type = "const " + Type;
			}

			if (!bIsFirstParam)
				Params += ", ";

			Params += Type + " " + Param.GetName();

			bIsFirstParam = false;
		}

		AllFuntionsText += std::format("\t{}{} {}({}){};\n", Func.IsStatic() ? "static " : "", RetType, Func.GetName(), Params, Func.IsConst() ? " const" : "");
	}

	return AllFuntionsText;
}

CppGenerator::FunctionInfo CppGenerator::GenerateFunctionInfo(const FunctionWrapper& Func)
{
	FunctionInfo RetFuncInfo;

	if (Func.IsPredefined())
	{
		RetFuncInfo.RetType = Func.GetPredefFuncReturnType();
		RetFuncInfo.FuncNameWithParams = Func.GetPredefFuncNameWithParams();

		return RetFuncInfo;
	}

	MemberManager FuncParams = Func.GetMembers();

	RetFuncInfo.FuncFlags = Func.GetFunctionFlags();
	RetFuncInfo.bIsReturningVoid = true;
	RetFuncInfo.RetType = "void";
	RetFuncInfo.FuncNameWithParams = Func.GetName() + "(";

	bool bIsFirstParam = true;

	RetFuncInfo.UnrealFuncParams.reserve(5);

	for (const PropertyWrapper& Param : FuncParams.IterateMembers())
	{
		std::string Type = GetMemberTypeString(Param);

		ParamInfo PInfo;
		PInfo.Type = Type;

		if (Param.IsReturnParam())
		{
			RetFuncInfo.RetType = GetMemberTypeString(Param);
			RetFuncInfo.bIsReturningVoid = false;

			PInfo.PropFlags = Param.GetPropertyFlags();
			PInfo.Name = Param.GetName();
			PInfo.Type = Type;
			PInfo.bIsRetParam = true;
			RetFuncInfo.UnrealFuncParams.push_back(PInfo);
			continue;
		}

		bool bIsRef = false;
		bool bIsOut = false;
		bool bIsConst = Param.HasPropertyFlags(EPropertyFlags::ConstParm);
		bool bIsMoveType = Param.IsType(EClassCastFlags::StructProperty | EClassCastFlags::ArrayProperty | EClassCastFlags::StrProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty);

		if (Param.HasPropertyFlags(EPropertyFlags::ReferenceParm))
		{
			Type += "&";
			bIsRef = true;
			bIsOut = true;
		}

		if (!bIsRef && Param.HasPropertyFlags(EPropertyFlags::OutParm))
		{
			Type += "*";
			bIsOut = true;
		}

		if (bIsConst)
			Type = "const " + Type;

		if (!bIsOut && !bIsRef && bIsMoveType)
		{
			Type += "&";

			if (!bIsConst)
				Type = "const " + Type;
		}

		std::string ParamName = Param.GetName();

		PInfo.bIsOutPtr = bIsOut && !bIsRef;
		PInfo.bIsOutRef = bIsOut && bIsRef;
		PInfo.bIsMoveParam = bIsMoveType;
		PInfo.bIsRetParam = false;
		PInfo.PropFlags = Param.GetPropertyFlags();
		PInfo.Name = ParamName;
		RetFuncInfo.UnrealFuncParams.push_back(PInfo);

		if (!bIsFirstParam)
			RetFuncInfo.FuncNameWithParams += ", ";

		RetFuncInfo.FuncNameWithParams += Type + " " + ParamName;

		bIsFirstParam = false;
	}

	RetFuncInfo.FuncNameWithParams += ")";

	return RetFuncInfo;
}

std::string CppGenerator::GenerateFunctions(const MemberManager& Members, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile)
{
	constexpr int32 AverageNumCharactersPerFunction = 0x50;

	std::string InHeaderFunctionText;

	bool bIsFirstIteration = true;
	bool bDidSwitch = false;
	bool bWasLastFuncStatic = false;
	bool bWasLastFuncInline = false;
	bool bWaslastFuncConst = false;

	for (const FunctionWrapper& Func : Members.IterateFunctions())
	{
		// Handeling spacing between static and non-static, const and non-const, as well as inline and non-inline functions
		if (bWasLastFuncInline != Func.HasInlineBody() && !bIsFirstIteration)
		{
			InHeaderFunctionText += "\npublic:\n";
			bDidSwitch = true;
		}

		if ((bWasLastFuncStatic != Func.IsStatic() || bWaslastFuncConst != Func.IsConst()) && !bIsFirstIteration && !bDidSwitch)
			InHeaderFunctionText += '\n';

		bWasLastFuncStatic = Func.IsStatic();
		bWasLastFuncInline = Func.HasInlineBody();
		bWaslastFuncConst = Func.IsConst();
		bIsFirstIteration = false;
		bDidSwitch = false;


		FunctionInfo FuncInfo = GenerateFunctionInfo(Func);

		const bool bHasInlineBody = Func.HasInlineBody();

		// Function declaration and inline-body generation
		InHeaderFunctionText += std::format("\t{}{} {}{}", Func.IsStatic() ? "static " : "", FuncInfo.RetType, FuncInfo.FuncNameWithParams, Func.IsConst() ? " const" : "");
		InHeaderFunctionText += (bHasInlineBody ? ("\n\t" + Func.GetPredefFunctionInlineBody()) : ";") + "\n";


		if (bHasInlineBody)
			continue;

		if (Func.IsPredefined())
		{
			std::string CustomComment = Func.GetPredefFunctionCustomComment();

			FunctionFile << std::format(R"(
// Predefined Function
{}
{} {}::{}
{}

)"
    , !CustomComment.empty() ? CustomComment + '\n' : ""
    , Func.GetPredefFuncReturnType()
    , StructName
    , Func.GetPredefFuncNameWithParams()
    , Func.GetPredefFunctionBody());

			continue;
		}


		// Parameter struct generation for unreal-functions
		if (!Func.IsPredefined() && Func.GetParamStructSize() > 0x0)
			GenerateStruct(Func.AsStruct(), ParamFile, FunctionFile, ParamFile);


		std::string ParamVarCreationString = std::format(R"(
	Params::{} Parms{{}};
)", Func.GetParamStructName());

		constexpr const char* StoreFunctionFlagsString = R"(
	auto Flgs = Func->FunctionFlags;
	Func->FunctionFlags |= 0x400;
)";

		std::string ParamDescriptionCommentString = "// Parameters:\n";
		std::string ParamAssignments;
		std::string OutPtrAssignments;
		std::string OutRefAssignments;

		const bool bHasParams = !FuncInfo.UnrealFuncParams.empty();
		bool bHasParamsToInit = false;
		bool bHasOutPtrParamsToInit = false;
		bool bHasOutRefParamsToInit = false;

		for (const ParamInfo& PInfo : FuncInfo.UnrealFuncParams)
		{
			ParamDescriptionCommentString += std::format("// {:{}}{:{}}({})\n", PInfo.Type, 40, PInfo.Name, 55, StringifyPropertyFlags(PInfo.PropFlags));

			if (PInfo.bIsRetParam)
				continue;

			if (PInfo.bIsOutPtr)
			{
				OutPtrAssignments += PInfo.bIsMoveParam ? std::format(R"(

	if ({0} != nullptr)
		*{0} = Parms.{0};)", PInfo.Name) : std::format(R"(

	if ({0} != nullptr)
		*{0} = std::move(Parms.{0});)", PInfo.Name);
				bHasOutPtrParamsToInit = true;
			}
			else
			{
				ParamAssignments += PInfo.bIsMoveParam ? std::format("\tParms.{0} = std::move({0});\n", PInfo.Name) : std::format("\tParms.{0} = {0};\n", PInfo.Name);
				bHasParamsToInit = true;
			}

			if (PInfo.bIsOutRef)
			{
				OutRefAssignments += PInfo.bIsMoveParam ? std::format("\n\t{0} = std::move(Parms.{0});", PInfo.Name) : std::format("\n\t{0} = Parms.{0};", PInfo.Name);
				bHasOutRefParamsToInit = true;
			}
		}

		//ParamAssignments = ParamAssignments + '\n';
		ParamAssignments = '\n' + ParamAssignments;
		//OutPtrAssignments = '\n' + OutPtrAssignments;
		OutRefAssignments = '\n' + OutRefAssignments;

		constexpr const char* RestoreFunctionFlagsString = R"(

	Func->FunctionFlags = Flgs;)";

		constexpr const char* ReturnValueString = R"(

	return Parms.ReturnValue;)";

		UEFunction UnrealFunc = Func.GetUnrealFunction();

		const bool bIsNativeFunc = Func.HasFunctionFlag(EFunctionFlags::Native);

		// Function implementation generation
		std::string FunctionImplementation = std::format(R"(
// {}
// ({})
{}
{} {}::{}
{{
	static class UFunction* Func = nullptr;

	if (Func == nullptr)
		Func = Class->GetFunction("{}", "{}");
{}{}{}
	UObject::ProcessEvent(Func, {});{}{}{}{}
}}

)"  , UnrealFunc.GetFullName()
    , StringifyFunctionFlags(FuncInfo.FuncFlags)
    , bHasParams ? ParamDescriptionCommentString : ""
    , FuncInfo.RetType
    , StructName
    , FuncInfo.FuncNameWithParams
    , UnrealFunc.GetOuter().GetName()
    , UnrealFunc.GetName()
    , bHasParams ? ParamVarCreationString : ""
    , bHasParamsToInit ? ParamAssignments : ""
    , bIsNativeFunc ? StoreFunctionFlagsString : ""
    , bHasParams ? "Parms" : "nullptr"
    , bIsNativeFunc ? RestoreFunctionFlagsString : ""
    , bHasOutRefParamsToInit ? OutRefAssignments : ""
    , bHasOutPtrParamsToInit ? OutPtrAssignments : ""
    , !FuncInfo.bIsReturningVoid ? ReturnValueString : "");

		FunctionFile << FunctionImplementation;
	}

	return InHeaderFunctionText;
}

void CppGenerator::GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile)
{
	if (!Struct.IsValid())
		return;

	std::string UniqueName = GetStructPrefixedName(Struct);
	std::string UniqueSuperName;

	int32 StructSize = Struct.GetSize();
	int32 SuperSize = 0x0;

	StructWrapper Super = Struct.GetSuper();

	if (Super.IsValid())
	{
		UniqueSuperName = GetStructPrefixedName(Super);
		SuperSize = Super.GetSize();
	}

	const bool bIsClass = Struct.IsClass();

	StructFile << std::format(R"(
// {}
// 0x{:04X} (0x{:04X} - 0x{:04X})
{} {}{}{}{}
{{
)", Struct.GetFullName()
, StructSize - SuperSize
, StructSize
, SuperSize
, bIsClass ? "class" : "struct"
, Struct.ShouldUseExplicitAlignment() ? std::format("alignas(0x{:02X}) ", Struct.GetAlignment()) : ""
, UniqueName
, Struct.IsFinal() ? " final " : ""
, Super.IsValid() ? (" : public " + UniqueSuperName) : "");

	MemberManager Members = Struct.GetMembers();

	const bool bHasMembers = Members.HasMembers();
	const bool bHasFunctions = Members.HasFunctions() && !Struct.IsFunction();

	if (bHasMembers || bHasFunctions)
		StructFile << "public:\n";

	if (bHasMembers)
	{
		StructFile << GenerateMembers(Struct, Members, SuperSize);

		if (bHasFunctions)
			StructFile << "\npublic:\n";
	}

	if (bHasFunctions)
		StructFile << GenerateFunctions(Members, UniqueName, FunctionFile, ParamFile);

	StructFile << "};\n";
}

void CppGenerator::GenerateEnum(const EnumWrapper& Enum, StreamType& StructFile)
{
	if (!Enum.IsValid())
		return;

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

	CollisionInfoIterator EnumValueIterator = Enum.GetMembers();

	int32 NumValues = 0x0;
	std::string MemberString;

	for (const EnumCollisionInfo& Info : EnumValueIterator)
	{
		NumValues++;
		MemberString += std::format("\t{:{}} = {},\n", Info.GetUniqueName(), 40, Info.GetValue());
	}

	if (!MemberString.empty()) [[likely]]
		MemberString.pop_back();

	std::string UnderlayingType = Enum.GetUnderlyingTypeSize() <= 0x8 ? UnderlayingTypesBySize[static_cast<size_t>(Enum.GetUnderlyingTypeSize()) - 1] : "uint8";

	StructFile << std::format(R"(
// {}
// NumValues: 0x{:04X}
enum class {} : {}
{{
{}
}};
)", Enum.GetFullName()
  , NumValues
  , GetEnumPrefixedName(Enum)
  , UnderlayingType
  , MemberString);
}


std::string CppGenerator::GetStructPrefixedName(const StructWrapper& Struct)
{
	if (Struct.IsFunction())
		return Struct.GetUnrealStruct().GetOuter().GetValidName() + "_" + Struct.GetName();

	auto [ValidName, bIsUnique] = Struct.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	/* Package::FStructName */
	return PackageManager::GetName(Struct.GetUnrealStruct().GetPackageIndex()) + "::" + ValidName;
}

std::string CppGenerator::GetEnumPrefixedName(const EnumWrapper& Enum)
{
	auto [ValidName, bIsUnique] = Enum.GetUniqueName();

	if (bIsUnique) [[likely]]
		return ValidName;

	/* Package::ESomeEnum */
	return PackageManager::GetName(Enum.GetUnrealEnum().GetPackageIndex()) + "::" + ValidName;
}

std::string CppGenerator::GetMemberTypeString(const PropertyWrapper& MemberWrapper)
{
	if (!MemberWrapper.IsUnrealProperty()) [[unlikely]]
		return MemberWrapper.GetType();

	return GetMemberTypeString(MemberWrapper.GetUnrealProperty());
}

std::string CppGenerator::GetMemberTypeString(UEProperty Member)
{
	auto [Class, FieldClass] = Member.GetClass();

	EClassCastFlags Flags = Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();

	if (Flags & EClassCastFlags::ByteProperty)
	{
		if (UEEnum Enum = Member.Cast<UEByteProperty>().GetEnum())
			return Enum.GetEnumTypeAsStr();

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
		if (Member.GetPropertyFlags() & EPropertyFlags::UObjectWrapper)
			return std::format("TSubclassOf<class {}>", GetStructPrefixedName(Member.Cast<UEClassProperty>().GetMetaClass()));

		return "class UClass*";
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return Member.Cast<UEBoolProperty>().IsNativeBool() ? "bool" : "uint8";
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		return std::format("struct {}", GetStructPrefixedName(Member.Cast<UEStructProperty>().GetUnderlayingStruct()));
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return std::format("TArray<{}>", GetMemberTypeString(Member.Cast<UEArrayProperty>().GetInnerProperty()));
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEWeakObjectProperty>().GetPropertyClass())
			return std::format("TWeakObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TWeakObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UELazyObjectProperty>().GetPropertyClass())
			return std::format("TLazyObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TLazyObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::SoftClassProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftClassProperty>().GetPropertyClass())
			return std::format("TSoftClassPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TSoftClassPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::SoftObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftObjectProperty>().GetPropertyClass())
			return std::format("TSoftObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TSoftObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::ObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEObjectProperty>().GetPropertyClass())
			return std::format("class {}*", GetStructPrefixedName(PropertyClass));

		return "class UObject*";
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		UEMapProperty MemberAsMapProperty = Member.Cast<UEMapProperty>();

		return std::format("TMap<{}, {}>", GetMemberTypeString(MemberAsMapProperty.GetKeyProperty()), GetMemberTypeString(MemberAsMapProperty.GetValueProperty()));
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return std::format("TSet<{}>", GetMemberTypeString(Member.Cast<UESetProperty>().GetElementProperty()));
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		if (UEEnum Enum = Member.Cast<UEEnumProperty>().GetEnum())
			return Enum.GetEnumTypeAsStr();

		return GetMemberTypeString(Member.Cast<UEEnumProperty>().GetUnderlayingProperty());
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEInterfaceProperty>().GetPropertyClass())
			return std::format("TScriptInterface<class {}>", GetStructPrefixedName(PropertyClass));

		return "TScriptInterface<class IInterface>";
	}
	else
	{
		return (Class ? Class.GetCppName() : FieldClass.GetCppName()) + "_";
	}
}

void CppGenerator::GenerateNameCollisionsInl(StreamType& NameCollisionsFile)
{
	NameCollisionsFile << R"(

//---------------------------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//---------------------------------------------------------------------------------------------------------------------

)";

	const StructManager::OverrideMaptType& StructInfoMap = StructManager::GetStructInfos();
	const EnumManager::OverrideMaptType& EnumInfoMap = EnumManager::GetEnumInfos();

	std::unordered_map<int32 /* PackageIdx */, std::pair<std::string, int32>> PackagesAndForwardDeclarations;

	for (const auto& [Index, Info] : StructInfoMap)
	{
		if (StructManager::IsStructNameUnique(Info.Name))
			continue;

		UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(Index);

		auto& [ForwardDeclarations, Count] = PackagesAndForwardDeclarations[Struct.GetPackageIndex()];

		ForwardDeclarations += std::format("\t{} {};\n", Struct.IsA(EClassCastFlags::Class) ? "class" : "struct", Struct.GetCppName());
		Count++;
	}

	for (const auto& [Index, Info] : EnumInfoMap)
	{
		if (EnumManager::IsEnumNameUnique(Info))
			continue;

		UEEnum Enum = ObjectArray::GetByIndex<UEEnum>(Index);

		auto& [ForwardDeclarations, Count] = PackagesAndForwardDeclarations[Enum.GetPackageIndex()];

		ForwardDeclarations += std::format("\t{} {};\n", "eunum class", Enum.GetEnumPrefixedName());
		Count++;
	}

	bool bHasSingleLineForwardDeclarations = false;

	for (const auto& [PackageIndex, ForwardDeclarations] : PackagesAndForwardDeclarations)
	{
		std::string ForwardDeclString = ForwardDeclarations.first.substr(0, ForwardDeclarations.first.size() - 1);
		std::string PackageName = PackageManager::GetName(PackageIndex);

		/* Only print packages with a single forward declaration at first */
		if (ForwardDeclarations.second > 1)
			continue;

		bHasSingleLineForwardDeclarations = true;

		NameCollisionsFile << std::format("\nnamespace {} {{ {} }}\n", PackageName, ForwardDeclString.c_str() + 1);
	}

	if (bHasSingleLineForwardDeclarations)
		NameCollisionsFile << "\n";

	for (const auto& [PackageIndex, ForwardDeclarations] : PackagesAndForwardDeclarations)
	{
		std::string ForwardDeclString = ForwardDeclarations.first.substr(0, ForwardDeclarations.first.size() - 1);
		std::string PackageName = PackageManager::GetName(PackageIndex);

		/* Now print all packages with several forward declarations */
		if (ForwardDeclarations.second <= 1)
			continue;

		NameCollisionsFile << std::format(R"(
namespace {}
{{
{}
}}
)", PackageName, ForwardDeclString);
	}

	NameCollisionsFile << std::endl;
}

void CppGenerator::Generate()
{
	// Launch NumberOfProcessorCores threads

	// Generate one package, open streams

	// Generate Basic.hpp and Basic.cpp files

	// Generate SDK.hpp with sorted packages

	// Generate NameCollisions.inl file containing forward declarations for classes in namespaces (potentially requires lock)
	StreamType NameCollisionsInl(MainFolder / "NameCollisions.inl");
	GenerateNameCollisionsInl(NameCollisionsInl);

	// Generate SDK

	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		std::string PackageName = PackageManager::GetName(Package);

		StreamType ClassesFile;
		StreamType StructsFile;
		StreamType ParametersFile;
		StreamType FunctionsFile;

		if (Package.HasClasses())
			ClassesFile = StreamType(Subfolder / (PackageName + "_classes.hpp"));

		if (Package.HasFunctions() || Package.HasEnums())
			StructsFile = StreamType(Subfolder / (PackageName + "_structs.hpp"));

		if (Package.HasParameterStructs())
			ParametersFile = StreamType(Subfolder / (PackageName + "_parameters.hpp"));

		if (Package.HasFunctions())
			FunctionsFile = StreamType(Subfolder / (PackageName + "_functions.cpp"));

		for (int32 EnumIdx : Package.GetEnums())
		{
			GenerateEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx), StructsFile);
		}

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			DependencyManager::IncludeFunctionType GenerateStructCallback = [&](int32 Index) -> void
			{
				GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructsFile, FunctionsFile, ParametersFile);
			};

			Structs.VisitAllNodesWithCallback(GenerateStructCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();

			DependencyManager::IncludeFunctionType GenerateClassCallback = [&](int32 Index) -> void
			{
				GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index), ClassesFile, FunctionsFile, ParametersFile);
			};

			Classes.VisitAllNodesWithCallback(GenerateClassCallback);
		}
	}

}

void CppGenerator::InitPredefinedMembers()
{

}

void CppGenerator::InitPredefinedFunctions()
{

}

