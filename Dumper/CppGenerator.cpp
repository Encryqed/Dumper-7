#include "CppGenerator.h"
#include "ObjectArray.h"
#include "MemberWrappers.h"
#include "MemberManager.h"

#include "SettingsRewrite.h"

constexpr std::string GetTypeFromSize(uint8 Size)
{
	switch (Size)
	{
	case 1:
		return "uint8";
	case 2:
		return "uint16";
	case 4:
		return "uint32";
	case 8:
		return "uint64";
	default:
		return "INVALID_TYPE_SIZE_FOR_BIT_PADDING";
		break;
	}
}

std::string CppGenerator::MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment)
{
	return std::format("\t{:{}} {:{}} // {}\n", Type, 45, Name + ";", 50, std::move(Comment));
}

std::string CppGenerator::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return MakeMemberString("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateBitPadding(uint8 UnderlayingSizeBytes, const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 BitPadNum = 0;

	return MakeMemberString(GetTypeFromSize(UnderlayingSizeBytes), std::format("BitPad_{:X} : {:X}", BitPadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, UnderlayingSizeBytes, std::move(Reason)));
}

std::string CppGenerator::GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize)
{
	constexpr uint64 EstimatedCharactersPerLine = 0x80;

	const bool bIsUnion = Struct.IsUnion();

	std::string OutMembers;
	OutMembers.reserve(Members.GetNumMembers() * EstimatedCharactersPerLine);

	bool bEncounteredStaticVariable = false;
	bool bAddedSpace = false;

	auto AddSpaceBetweenSticAndNormalMembers = [&](const PropertyWrapper& Member)
	{
		if (!bAddedSpace && bEncounteredStaticVariable && !Member.IsStatic())
		{
			OutMembers += '\n';
			bAddedSpace = true;
		}

		bEncounteredStaticVariable = Member.IsStatic();
	};

	bool bLastPropertyWasBitField = false;

	int PrevPropertyEnd = SuperSize;
	int PrevBitPropertyEnd = 0;
	int PrevBitPropertyEndBit = 1;

	uint8 PrevBitPropertySize = 0x1;
	uint8 PrevBitPropertyOffset = 0x0;
	uint64 PrevMaxIndexInUnderlayingType = 0x8;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		AddSpaceBetweenSticAndNormalMembers(Member);

		const int32 MemberOffset = Member.GetOffset();
		const int32 MemberSize = Member.GetSize();

		const int32 CurrentPropertyEnd = MemberOffset + MemberSize;

		std::string Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, Member.GetFlagsOrCustomComment());

		const bool bIsBitField = Member.IsBitField();

		/* Padding between two bitfields at different byte-offsets */
		if (CurrentPropertyEnd > PrevPropertyEnd && bLastPropertyWasBitField && bIsBitField && PrevBitPropertyEndBit < PrevMaxIndexInUnderlayingType && !bIsUnion)
		{
			OutMembers += GenerateBitPadding(PrevBitPropertySize, PrevBitPropertyOffset, (PrevMaxIndexInUnderlayingType + 1) - PrevBitPropertyEndBit, "Fixing Bit-Field Size For New Byte [ Dumper-7 ]");
			PrevBitPropertyEndBit = 0;
		}

		if (MemberOffset > PrevPropertyEnd && !bIsUnion)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, MemberOffset - PrevPropertyEnd, "Fixing Size After Last Property [ Dumper-7 ]");

		if (bIsBitField)
		{
			uint8 BitFieldIndex = Member.GetBitIndex();
			uint8 BitSize = Member.GetBitCount();

			std::string BitfieldInfoComment = std::format("BitIndex: 0x{:02X}, PropSize: 0x{:04X} ({})", BitFieldIndex, MemberSize, Member.GetFlagsOrCustomComment());
			Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, BitfieldInfoComment);

			if (PrevBitPropertyEnd < MemberOffset)
				PrevBitPropertyEndBit = 0;

			if (PrevBitPropertyEndBit < BitFieldIndex && !bIsUnion)
				OutMembers += GenerateBitPadding(MemberSize, MemberOffset, BitFieldIndex - PrevBitPropertyEndBit, "Fixing Bit-Field Size Between Bits [ Dumper-7 ]");

			PrevBitPropertyEndBit = BitFieldIndex + BitSize;
			PrevBitPropertyEnd = MemberOffset  + MemberSize;

			PrevBitPropertySize = MemberSize;
			PrevBitPropertyOffset = MemberOffset;

			PrevMaxIndexInUnderlayingType = (MemberSize * 0x8) - 1;
		}

		bLastPropertyWasBitField = bIsBitField;

		if (!Member.IsStatic()) [[likely]]
			PrevPropertyEnd = MemberOffset + (MemberSize * Member.GetArrayDim());

		std::string MemberName = Member.GetName();

		if (Member.GetArrayDim() > 1)
		{
			MemberName += std::format("[0x{:X}]", Member.GetArrayDim());
		}
		else if (bIsBitField)
		{
			MemberName += (" : " + std::to_string(Member.GetBitCount()));
		}

		if (Member.HasDefaultValue()) [[unlikely]]
			MemberName += (" = " + Member.GetDefaultValue());

		OutMembers += MakeMemberString(GetMemberTypeString(Member), MemberName, std::move(Comment));
	}

	const int32 MissingByteCount = Struct.GetSize() - PrevPropertyEnd;

	if (MissingByteCount >= Struct.GetAlignment())
		OutMembers += GenerateBytePadding(PrevPropertyEnd, MissingByteCount, "Fixing Struct Size After Last Property [ Dumper-7 ]");

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

std::string CppGenerator::GenerateSingleFunction(const FunctionWrapper& Func, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile)
{
	namespace CppSettings = SettingsRewrite::CppGenerator;

	std::string InHeaderFunctionText;

	FunctionInfo FuncInfo = GenerateFunctionInfo(Func);

	const bool bHasInlineBody = Func.HasInlineBody();

	// Function declaration and inline-body generation
	InHeaderFunctionText += std::format("\t{}{} {}{}", Func.IsStatic() ? "static " : "", FuncInfo.RetType, FuncInfo.FuncNameWithParams, Func.IsConst() ? " const" : "");
	InHeaderFunctionText += (bHasInlineBody ? ("\n\t" + Func.GetPredefFunctionInlineBody()) : ";") + "\n";


	if (bHasInlineBody)
		return InHeaderFunctionText;

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

		return InHeaderFunctionText;
	}


	// Parameter struct generation for unreal-functions
	if (!Func.IsPredefined() && Func.GetParamStructSize() > 0x0)
		GenerateStruct(Func.AsStruct(), ParamFile, FunctionFile, ParamFile);


	std::string ParamVarCreationString = std::format(R"(
	{}{} Parms{{}};
)", CppSettings::ParamNamespaceName ? std::format("{}::", CppSettings::ParamNamespaceName) : "", Func.GetParamStructName());

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

	ParamAssignments = '\n' + ParamAssignments;
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

)", UnrealFunc.GetFullName()
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

	return InHeaderFunctionText;
}

std::string CppGenerator::GenerateFunctions(const StructWrapper& Struct, const MemberManager& Members, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile)
{
	namespace CppSettings = SettingsRewrite::CppGenerator;

	static PredefinedFunction StaticClass;
	static PredefinedFunction GetDefaultObj;

	if (StaticClass.NameWithParams.empty())
		StaticClass = {
		.CustomComment = "Used with 'IsA' to check if an object is of a certain type",
		.ReturnType = "class UClass*",
		.NameWithParams = "StaticClass()",

		.bIsStatic = true,
		.bIsConst = false,
		.bIsBodyInline = true,
	};

	if (GetDefaultObj.NameWithParams.empty())
		GetDefaultObj = {
		.CustomComment = "Only use the default object to call \"static\" functions",
		.NameWithParams = "GetDefaultObj()",

		.bIsStatic = true,
		.bIsConst = false,
		.bIsBodyInline = true,
	};

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

		InHeaderFunctionText += GenerateSingleFunction(Func, StructName, FunctionFile, ParamFile);
	}

	if (!Struct.IsUnrealStruct() || !Struct.IsClass())
		return InHeaderFunctionText;

	/* Special spacing for UClass specific functions 'StaticClass' and 'GetDefaultObj' */
	if (bWasLastFuncInline != StaticClass.bIsBodyInline && !bIsFirstIteration)
	{
		InHeaderFunctionText += "\npublic:\n";
		bDidSwitch = true;
	}

	if ((bWasLastFuncStatic != StaticClass.bIsStatic || bWaslastFuncConst != StaticClass.bIsConst) && !bIsFirstIteration && !bDidSwitch)
		InHeaderFunctionText += '\n';

	const bool bIsNameUnique = Struct.GetUniqueName().second;

	std::string Name = !bIsNameUnique ? Struct.GetFullName() : Struct.GetRawName();
	std::string NameText = CppSettings::XORString ? std::format("{}(\"{}\")", CppSettings::XORString, Name) : std::format("\"{}\"", Name);

	/* Set class-specific parts of 'StaticClass' and 'GetDefaultObj' */
	StaticClass.Body = std::format(
R"({{
	return StaticClassImpl<{}{}>();
}})", NameText, (!bIsNameUnique ? ", true" : ""));

	GetDefaultObj.ReturnType = std::format("class {}*", StructName);
	GetDefaultObj.Body = std::format(
R"({{
	return GetDefaultObjImpl<{}>();
}})", StructName);


	std::shared_ptr<StructWrapper> CurrentStructPtr = std::make_shared<StructWrapper>(Struct);
	InHeaderFunctionText += GenerateSingleFunction(FunctionWrapper(CurrentStructPtr, &StaticClass), StructName, FunctionFile, ParamFile);
	InHeaderFunctionText += GenerateSingleFunction(FunctionWrapper(CurrentStructPtr, &GetDefaultObj), StructName, FunctionFile, ParamFile);

	return InHeaderFunctionText;
}

void CppGenerator::GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile)
{
	if (!Struct.IsValid())
		return;

	std::string UniqueName = GetStructPrefixedName(Struct);
	std::string UniqueSuperName;

	const int32 StructSize = Struct.GetSize();
	int32 SuperSize = 0x0;

	StructWrapper Super = Struct.GetSuper();

	if (Super.IsValid())
	{
		UniqueSuperName = GetStructPrefixedName(Super);
		SuperSize = Super.GetSize();
	}

	const int32 StructSizeWithoutSuper = StructSize - SuperSize;

	const bool bIsClass = Struct.IsClass();
	const bool bIsUnion = Struct.IsUnion();

	StructFile << std::format(R"(
// {}
// 0x{:04X} (0x{:04X} - 0x{:04X})
{} {}{}{}{}
{{
)", Struct.GetFullName()
  , StructSizeWithoutSuper
  , StructSize
  , SuperSize
  , bIsClass ? "class" : (bIsUnion ? "union" : "struct")
  , Struct.ShouldUseExplicitAlignment() ? std::format("alignas(0x{:02X}) ", Struct.GetAlignment()) : ""
  , UniqueName
  , Struct.IsFinal() ? " final " : ""
  , Super.IsValid() ? (" : public " + UniqueSuperName) : "");

	MemberManager Members = Struct.GetMembers();

	const bool bHasStaticClass = (bIsClass && Struct.IsUnrealStruct());

	const bool bHasMembers = Members.HasMembers() || (StructSizeWithoutSuper >= Struct.GetAlignment());
	const bool bHasFunctions = (Members.HasFunctions() && !Struct.IsFunction()) || bHasStaticClass;

	if (bHasMembers || bHasFunctions)
		StructFile << "public:\n";

	if (bHasMembers)
	{
		StructFile << GenerateMembers(Struct, Members, SuperSize);

		if (bHasFunctions)
			StructFile << "\npublic:\n";
	}

	if (bHasFunctions)
		StructFile << GenerateFunctions(Struct, Members, UniqueName, FunctionFile, ParamFile);

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
	if (!MemberWrapper.IsUnrealProperty())
	{
		if (MemberWrapper.IsStatic() && !MemberWrapper.IsZeroSizedMember())
			return "static " + MemberWrapper.GetType();

		return MemberWrapper.GetType();
	}

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
	namespace CppSettings = SettingsRewrite::CppGenerator;

	WriteFileHead(NameCollisionsFile, nullptr, EFileType::NameCollisionsInl, "FORWARD DECLARATIONS");


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

	WriteFileEnd(NameCollisionsFile, EFileType::NameCollisionsInl);
}

void CppGenerator::WriteFileHead(StreamType& File, PackageInfoHandle Package, EFileType Type, const std::string& CustomFileComment, const std::string& CustomIncludes)
{
	namespace CppSettings = SettingsRewrite::CppGenerator;

	File << std::format(R"(#pragma once

/*
* SDK generated by Dumper-7
*
* https://github.com/Encryqed/Dumper-7
*/

// {}

)", Package.IsValidHandle() ? std::format("Package: {}", Package.GetName()) : CustomFileComment);

	if (!CustomIncludes.empty())
		File << CustomIncludes + "\n";

	if (Type != EFileType::BasicHpp && Type != EFileType::NameCollisionsInl)
		File << "#include \"Basic.hpp\";\n";

	if (Type == EFileType::BasicHpp)
		File << "#include \"NameCollisions.inl\";\n";

	if (Type == EFileType::Functions && Package.HasParameterStructs())
	{
		File << "\n";
		File << std::format("#include \"{}_parameters.hpp\"\n", Package.GetName());
		File << "\n";
	}
	else if (Package.IsValidHandle())
	{
		File << "\n";

		const DependencyInfo& Dep = Package.GetPackageDependencies();
		const DependencyListType& CurrentDependencyList = Type == EFileType::Structs ? Dep.StructsDependencies : (Type == EFileType::Classes ? Dep.ClassesDependencies : Dep.ParametersDependencies);

		bool bAddNewLine = false;

		for (const auto& [PackageIndex, Requirements] : CurrentDependencyList)
		{
			bAddNewLine = true;

			std::string DependencyName = PackageManager::GetName(PackageIndex);

			if (Requirements.bShouldIncludeStructs)
			{
				File << std::format("#include \"{}_structs.hpp\"\n", DependencyName);
			}

			if (Requirements.bShouldIncludeClasses)
			{
				File << std::format("#include \"{}_classes.hpp\"\n", DependencyName);
			}
		}

		if (bAddNewLine)
			File << "\n";
	}

	File << "\n";

	if constexpr (CppSettings::SDKNamespaceName)
	{
		File << std::format("namespace {}", CppSettings::SDKNamespaceName);

		if (Type == EFileType::Parameters && CppSettings::ParamNamespaceName)
			File << std::format("::{}", CppSettings::ParamNamespaceName);

		File << "\n{\n";
	}
	else if constexpr (CppSettings::ParamNamespaceName)
	{
		if (Type == EFileType::Parameters)
			File << std::format("namespace {}\n{{\n", CppSettings::ParamNamespaceName);
	}
}

void CppGenerator::WriteFileEnd(StreamType& File, EFileType Type)
{
	namespace CppSettings = SettingsRewrite::CppGenerator;

	if constexpr (CppSettings::SDKNamespaceName || CppSettings::ParamNamespaceName)
	{
		if (Type != EFileType::Functions)
			File << "\n";

		File << "}\n\n";
	}
}

void CppGenerator::Generate()
{
	// Generate Basic.hpp and Basic.cpp files
	StreamType BasicHpp(Subfolder / "Basic.hpp");
	StreamType BasicCpp(Subfolder / "Basic.cpp");
	GenerateBasicFiles(BasicHpp, BasicCpp);

	// Generate SDK.hpp with sorted packages

	// Generate NameCollisions.inl file containing forward declarations for classes in namespaces (potentially requires lock)
	StreamType NameCollisionsInl(MainFolder / "NameCollisions.inl");
	GenerateNameCollisionsInl(NameCollisionsInl);

	// Generate one package, open streams
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		std::string FileName = SettingsRewrite::CppGenerator::FilePrefix + Package.GetName();

		StreamType ClassesFile;
		StreamType StructsFile;
		StreamType ParametersFile;
		StreamType FunctionsFile;

		if (Package.HasClasses())
		{
			ClassesFile = StreamType(Subfolder / (FileName + "_classes.hpp"));
			WriteFileHead(ClassesFile, Package, EFileType::Classes);
		}

		if (Package.HasStructs() || Package.HasEnums())
		{
			StructsFile = StreamType(Subfolder / (FileName + "_structs.hpp"));
			WriteFileHead(StructsFile, Package, EFileType::Structs);
		}

		if (Package.HasParameterStructs())
		{
			ParametersFile = StreamType(Subfolder / (FileName + "_parameters.hpp"));
			WriteFileHead(ParametersFile, Package, EFileType::Parameters);
		}

		if (Package.HasFunctions())
		{
			FunctionsFile = StreamType(Subfolder / (FileName + "_functions.cpp"));
			WriteFileHead(FunctionsFile, Package, EFileType::Functions);
		}


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


		if (Package.HasClasses())
			WriteFileEnd(ClassesFile, EFileType::Classes);

		if (Package.HasStructs() || Package.HasEnums())
			WriteFileEnd(StructsFile, EFileType::Structs);

		if (Package.HasParameterStructs())
			WriteFileEnd(ParametersFile, EFileType::Parameters);

		if (Package.HasFunctions())
			WriteFileEnd(FunctionsFile, EFileType::Functions);
	}

}

void CppGenerator::InitPredefinedMembers()
{
	static auto SortMembers = [](std::vector<PredefinedMember>& Members) -> void
	{
		std::sort(Members.begin(), Members.end(), ComparePredefinedMembers);
	};

	/* Assumes Members to be sorted */
	static auto InitStructSize = [](PredefinedStruct& Struct) -> void
	{
		if (Struct.Size > 0x0)
			return;

		if (Struct.Properties.empty() && Struct.Super)
			Struct.Size = Struct.Super->Size;

		const PredefinedMember& LastMember = Struct.Properties[Struct.Properties.size() - 1];
		Struct.Size = LastMember.Offset + LastMember.Size;
	};
	PredefinedElements& ULevelPredefs = PredefinedMembers[ObjectArray::FindClassFast("Level").GetIndex()];
	ULevelPredefs.Members =
	{
		PredefinedMember {
			.Comment = "THIS IS THE ARRAY YOU'RE LOOKING FOR! [NOT AUTO-GENERATED PROPERTY]",
			.Type = "class TArray<class AActor*>", .Name = "Actors", .Offset = Off::ULevel::Actors, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UObjectPredefs = PredefinedMembers[ObjectArray::FindClassFast("Object").GetIndex()];
	UObjectPredefs.Members = 
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class TUObjectArrayWrapper", .Name = "GObjects", .Offset = 0x0, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = true, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "void*", .Name = "VTable", .Offset = Off::UObject::Vft, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Flags", .Offset = Off::UObject::Flags, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Index", .Offset = Off::UObject::Index, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "Class", .Offset = Off::UObject::Class, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FName", .Name = "Name", .Offset = Off::UObject::Name, .Size = Off::InSDK::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "Outer", .Offset = Off::UObject::Outer, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UFieldPredefs = PredefinedMembers[ObjectArray::FindClassFast("Field").GetIndex()];
	UFieldPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UField*", .Name = "Next", .Offset = Off::UField::Next, .Size = 0x8, .ArrayDim = 0x1, .Alignment =0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UEnumPredefs = PredefinedMembers[ObjectArray::FindClassFast("Enum").GetIndex()];
	UEnumPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class TArray<class TPair<class FName, int64>>", .Name = "Names", .Offset = Off::UEnum::Names, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UStructPredefs = PredefinedMembers[ObjectArray::FindClassFast("Struct").GetIndex()];
	UStructPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UStruct*", .Name = "Super", .Offset = Off::UStruct::SuperStruct, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UField*", .Name = "Children", .Offset = Off::UStruct::Children, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "MinAlignemnt", .Offset = Off::UStruct::MinAlignemnt, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Size", .Offset = Off::UStruct::Size, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
		},
	};

	if (Settings::Internal::bUseFProperty)
	{
		UStructPredefs.Members.push_back({
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FField*", .Name = "ChildProperties", .Offset = Off::UStruct::ChildProperties, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		});
	}

	PredefinedElements& UFunctionPredefs = PredefinedMembers[ObjectArray::FindClassFast("Function").GetIndex()];
	UFunctionPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "using FNativeFuncPtr = void (*)(void* Context, void* TheStack, void* Result)", .Name = "", .Offset = 0x0, .Size = 0x00, .ArrayDim = 0x1, .Alignment = 0x0,
			.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint32", .Name = "FunctionFlags", .Offset = Off::UFunction::FunctionFlags, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "FNativeFuncPtr", .Name = "ExecFunction", .Offset = Off::UFunction::ExecFunction, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UClassPredefs = PredefinedMembers[ObjectArray::FindClassFast("Class").GetIndex()];
	UClassPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "enum class EClassCastFlags", .Name = "CastFlags", .Offset = Off::UClass::CastFlags, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "DefaultObject", .Offset = Off::UClass::ClassDefaultObject, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::string PropertyTypePtr = Settings::Internal::bUseFProperty ? "class FProperty*" : "class UProperty*";

	std::vector<PredefinedMember> PropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ArrayDim", .Offset = Off::UProperty::ArrayDim, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ElementSize", .Offset = Off::UProperty::ElementSize, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,  .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint64", .Name = "PropertyFlags", .Offset = Off::UProperty::PropertyFlags, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Offset", .Offset = Off::UProperty::Offset_Internal, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> BytePropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UEnum*", .Name = "Enum", .Offset = Off::UByteProperty::Enum, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> BoolPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "FieldSize", .Offset = Off::UBoolProperty::Base, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "ByteOffset", .Offset = Off::UBoolProperty::Base + 0x1, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false,  .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "ByteMask", .Offset = Off::UBoolProperty::Base + 0x2, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "FieldMask", .Offset = Off::UBoolProperty::Base + 0x3, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ObjectPropertyBaseMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "PropertyClass", .Offset = Off::UObjectProperty::PropertyClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ClassPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "MetaClass", .Offset = Off::UClassProperty::MetaClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> StructPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UStruct*", .Name = "Struct", .Offset = Off::UStructProperty::Struct, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ArrayPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "InnerProperty", .Offset = Off::UArrayProperty::Inner, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> MapPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "KeyProperty", .Offset = Off::UMapProperty::Base, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "ValueProperty", .Offset = Off::UMapProperty::Base + 0x8, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> SetPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "ElementProperty", .Offset = Off::USetProperty::ElementProp, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> EnumPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "UnderlayingProperty", .Offset = Off::UEnumProperty::Base, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UEnum*", .Name = "Enum", .Offset = Off::UEnumProperty::Base + 0x8, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	SortMembers(PropertyMembers);
	SortMembers(BytePropertyMembers);
	SortMembers(BoolPropertyMembers);
	SortMembers(ObjectPropertyBaseMembers);
	SortMembers(ClassPropertyMembers);
	SortMembers(StructPropertyMembers);
	SortMembers(ArrayPropertyMembers);
	SortMembers(MapPropertyMembers);
	SortMembers(SetPropertyMembers);
	SortMembers(EnumPropertyMembers);

	if (!Settings::Internal::bUseFProperty)
	{
		PredefinedMembers[ObjectArray::FindClassFast("Property").GetIndex()].Members = PropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("ByteProperty").GetIndex()].Members = BytePropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("BoolProperty").GetIndex()].Members = BoolPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("ObjectPropertyBase").GetIndex()].Members = ObjectPropertyBaseMembers;
		PredefinedMembers[ObjectArray::FindClassFast("ClassProperty").GetIndex()].Members = ClassPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("StructProperty").GetIndex()].Members = StructPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("ArrayProperty").GetIndex()].Members = ArrayPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("MapProperty").GetIndex()].Members = MapPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("SetProperty").GetIndex()].Members = SetPropertyMembers;
		PredefinedMembers[ObjectArray::FindClassFast("EnumProperty").GetIndex()].Members = EnumPropertyMembers;
	}
	else
	{
		/* Reserving enough space is required because otherwise the vector could reallocate and invalidate some structs' 'Super' pointer */
		PredefinedStructs.reserve(0x20);

		PredefinedStruct& FFieldClass = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FFieldClass", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = nullptr
		});
		PredefinedStruct& FFieldVariant = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FFieldVariant", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = nullptr
		});

		PredefinedStruct& FField = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FField", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = nullptr
		});

		PredefinedStruct& FProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FProperty", .Size = Off::InSDK::PropertySize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = &FField  /* FField */
		});
		PredefinedStruct& FByteProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FByteProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FBoolProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FBoolProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FObjectPropertyBase = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FObjectPropertyBase", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FClassProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FClassProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FObjectPropertyBase  /* FObjectPropertyBase */
		});
		PredefinedStruct& FStructProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FStructProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FArrayProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FArrayProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FMapProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FMapProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FSetProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FSetProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FEnumProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FEnumProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});

		FFieldClass.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "FName", .Name = "Name", .Offset = Off::FFieldClass::Name, .Size = Off::InSDK::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint64", .Name = "Id", .Offset = Off::FFieldClass::Id, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint64", .Name = "CastFlags", .Offset = Off::FFieldClass::CastFlags, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "EClassFlags", .Name = "ClassFlags", .Offset = Off::FFieldClass::ClassFlags, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class FFieldClass*", .Name = "SuperClass", .Offset = Off::FFieldClass::SuperClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};


		const int32 FFieldVariantSize = Settings::Internal::bUseMaskForFieldOwner ? 0x8 : 0x10;
		const int32 IdentifierOffset = Settings::Internal::bUseMaskForFieldOwner ? 0x0 : 0x8;
		std::string UObjectIdentifierType = (Settings::Internal::bUseMaskForFieldOwner ? "static constexpr uint64" : "bool");
		std::string UObjectIdentifierName = (Settings::Internal::bUseMaskForFieldOwner ? "UObjectMask = 0x1" : "bIsUObject");

		FFieldVariant.Properties =
		{
			// continue here
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "union { class FField* Field; class UObject* Object; }", .Name = "Container", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = UObjectIdentifierType, .Name = UObjectIdentifierName, .Offset = IdentifierOffset, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = Settings::Internal::bUseMaskForFieldOwner, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};


		FField.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "void*", .Name = "VTable", .Offset = Off::FField::Vft, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false,.bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class FFieldClass*", .Name = "ClassPrivate", .Offset = Off::FField::Class, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "FFieldVariant", .Name = "Owner", .Offset = Off::FField::Owner, .Size = FFieldVariantSize, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class FField*", .Name = "Next", .Offset = Off::FField::Next, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "FName", .Name = "Name", .Offset = Off::FField::Name, .Size = Off::InSDK::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "ObjFlags", .Offset = Off::FField::Flags, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		SortMembers(FFieldClass.Properties);
		SortMembers(FFieldVariant.Properties);

		SortMembers(FField.Properties);

		FProperty.Properties = PropertyMembers;
		FByteProperty.Properties = BytePropertyMembers;
		FBoolProperty.Properties = BoolPropertyMembers;
		FObjectPropertyBase.Properties = ObjectPropertyBaseMembers;
		FClassProperty.Properties = ClassPropertyMembers;
		FStructProperty.Properties = StructPropertyMembers;
		FArrayProperty.Properties = ArrayPropertyMembers;
		FMapProperty.Properties = MapPropertyMembers;
		FSetProperty.Properties = SetPropertyMembers;
		FEnumProperty.Properties = EnumPropertyMembers;

		/* Init PredefindedStruct::Size **after** sorting the members */
		InitStructSize(FFieldClass);
		InitStructSize(FFieldVariant);

		InitStructSize(FField);

		InitStructSize(FProperty);
		InitStructSize(FByteProperty);
		InitStructSize(FBoolProperty);
		InitStructSize(FObjectPropertyBase);
		InitStructSize(FClassProperty);
		InitStructSize(FStructProperty);
		InitStructSize(FArrayProperty);
		InitStructSize(FMapProperty);
		InitStructSize(FSetProperty);
		InitStructSize(FEnumProperty);
	}

	SortMembers(UObjectPredefs.Members);
	SortMembers(UFieldPredefs.Members);
	SortMembers(UEnumPredefs.Members);
	SortMembers(UStructPredefs.Members);
	SortMembers(UFunctionPredefs.Members);
	SortMembers(UClassPredefs.Members);
}

void CppGenerator::InitPredefinedFunctions()
{
	static auto SortFunctions = [](std::vector<PredefinedFunction>& Functions) -> void
	{
		std::sort(Functions.begin(), Functions.end(), ComparePredefinedFunctions);
	};

}

namespace Offsets
{
	int GObjects;
}

void CppGenerator::GenerateBasicFiles(StreamType& BasicHpp, StreamType& BasicCpp)
{
	namespace CppSettings = SettingsRewrite::CppGenerator;

	std::string CustomIncludes = R"(#include <string>
#include <Windows.h>
#include <iostream>
#include <type_traits>
)";

	WriteFileHead(BasicHpp, nullptr, EFileType::BasicHpp, "Basic file containing structs required by the SDK", CustomIncludes);
	WriteFileHead(BasicCpp, nullptr, EFileType::BasicCpp, "Basic file containing function-implementations from Basic.hpp");


	/* typedefs for integer types (eg. uint8, int32, etc.) */
	BasicHpp <<
		R"(
typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
)";


	/* Offsets and disclaimer */
	BasicHpp << std::format(R"(
/*
* Disclaimer:
*	- The 'GNames' is only a fallback and null by default, FName::AppendString is used
*	- THe 'GWorld' offset is not found automatically, use the provided 'UWorld::GetWorld()' function instead
*/
namespace Offsets
{{
	constexpr int32 GObjects          = 0x{:08X};
	constexpr int32 AppendString      = 0x{:08X};
	constexpr int32 GNames            = 0x{:08X};
	constexpr int32 ProcessEvent      = 0x{:08X};
	constexpr int32 ProcessEventIdx   = 0x{:08X};
}}
)", Off::InSDK::GObjects, Off::InSDK::AppendNameToString, Off::InSDK::GNames, Off::InSDK::PEOffset, Off::InSDK::PEIndex);



	// Start Namespace 'InSDKUtils'
	BasicHpp <<
		R"(
namespace InSDKUtils
{
)";


	/* Custom 'GetImageBaseInSDK' function */
	BasicHpp << std::format(R"(	inline void* GetImageBase()
{}
)", CppSettings::GetImageBaseFuncBody);


	/* GetVirtualFunction(const void* ObjectInstance, int32 Index) function */
	BasicHpp << R"(	template<typename FuncType>
	inline FuncType GetVirtualFunction(const void* ObjectInstance, int32 Index)
	{
		void** VTable = *reinterpret_cast<const void***>(const_cast<void*>(ObjectInstance));

		return reinterpret_cast<FuncType>(VTable[Index]);
	}
)";

	BasicHpp << "}\n\n";
	// End Namespace 'InSDKUtils'



	// Start class 'TUObjectArray'
	PredefinedStruct FUObjectItem = PredefinedStruct{
		.UniqueName = "FUObjectItem", .Size = Off::InSDK::FUObjectItemSize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr  /* FProperty */
	};

	FUObjectItem.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "Object", .Offset = Off::InSDK::FUObjectItemInitialOffset, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	GenerateStruct(&FUObjectItem, BasicHpp, BasicCpp, BasicHpp);


	constexpr const char* DefaultDecryption = R"([](void* ObjPtr) -> uint8*
	{
		return reinterpret_cast<uint8*>(ObjPtr);
	})";

	std::string DecryptionStrToUse = ObjectArray::DecryptionLambdaStr.empty() ? DefaultDecryption : std::move(ObjectArray::DecryptionLambdaStr);


	if (Off::InSDK::ChunkSize <= 0)
	{
		BasicHpp << std::format(R"(
class TUObjectArray
{{
private:
	static inline auto DecryptPtr = {};

public:
	FUObjectItem* Objects;
	int32 MaxElements;
	int32 NumElements;

public:
	// Call InitGObjects() before using these functions
	inline int Num() const
	{{
		return NumElements;
	}}

	inline FUObjectItem* GetDecrytedObjPtr() const
	{{
		return reinterpret_cast<FUObjectItem*>(DecryptPtr(Objects));
	}}

	inline class UObject* GetByIndex(const int32 Index) const
	{{
		if (Index < 0 || Index > NumElements)
			return nullptr;

		return GetDecrytedObjPtr()[Index].Object;
	}}
}};
)", DecryptionStrToUse);
	}
	else
	{
		constexpr const char* MemmberString = R"(
	FUObjectItem** Objects;
	uint8 Pad_0[0x08];
	int32 MaxElements;
	int32 NumElements;
	int32 MaxChunks;
	int32 NumChunks;)";

		constexpr const char* MemberStringWeirdLayout = R"(
	uint8 Pad_0[0x10];
	int32 MaxElements;
	int32 NumElements;
	int32 MaxChunks;
	int32 NumChunks;
	FUObjectItem** Objects;)";

		BasicHpp << std::format(R"(
class TUObjectArray
{{
public:
	enum
	{{
		ElementsPerChunk = 0x{:X},
	}};

private:
	static inline auto DecryptPtr = {};

public:{}

public:
	inline int32 Num() const
	{{
		return NumElements;
	}}

	inline FUObjectItem** GetDecrytedObjPtr() const
	{{
		return reinterpret_cast<FUObjectItem**>(DecryptPtr(Objects));
	}}

	inline class UObject* GetByIndex(const int32 Index) const
	{{
		if (Index < 0 || Index > NumElements)
			return nullptr;

		const int32 ChunkIndex = Index / ElementsPerChunk;
		const int32 InChunkIdx = Index % ElementsPerChunk;

		return GetDecrytedObjPtr()[ChunkIndex][InChunkIdx].Object;
	}}
}};
)", Off::InSDK::ChunkSize, DecryptionStrToUse, Off::FUObjectArray::Ptr == 0 ? MemmberString : MemberStringWeirdLayout);
	}
	// End class 'TUObjectArray'



	/* TUObjectArrayWrapper so InitGObjects() doesn't need to be called manually anymore */
	// Start class 'TUObjectArrayWrapper'
		BasicHpp << R"(
class TUObjectArrayWrapper
{
private:
	void* GObjectsAddress = nullptr;

public:
	TUObjectArrayWrapper() = delete;
	TUObjectArrayWrapper(TUObjectArrayWrapper&&) = delete;
	TUObjectArrayWrapper(const TUObjectArrayWrapper&) = delete;

	TUObjectArrayWrapper& operator=(TUObjectArrayWrapper&&) = delete;
	TUObjectArrayWrapper& operator=(const TUObjectArrayWrapper&) = delete;

private:
	inline void InitGObjects()
	{
		GObjectsAddress = reinterpret_cast<void*>(uintptr_t(InSDKUtils::GetImageBase()) + Offsets::GObjects);
	}

public:
	inline class TUObjectArray* operator->()
	{
		if (!GObjectsAddress) [[unlikely]]
			InitGObjects();

		return reinterpret_cast<class TUObjectArray*>(GObjectsAddress);
	}
};
)";
	// End class 'TUObjectArrayWrapper'



	// Start class 'TArray/FString'
	BasicHpp << R"(
template<class T>
class TArray
{
protected:
	T* Data;
	int32 NumElements;
	int32 MaxElements;

public:

	inline TArray()
		:NumElements(0), MaxElements(0), Data(nullptr)
	{
	}

	inline TArray(int32 Size)
		:NumElements(0), MaxElements(Size), Data(reinterpret_cast<T*>(malloc(sizeof(T) * Size)))
	{
	}

	inline T& operator[](uint32 Index)
	{
		return Data[Index];
	}
	inline const T& operator[](uint32 Index) const
	{
		return Data[Index];
	}

	inline int32 Num()
	{
		return NumElements;
	}

	inline int32 Max()
	{
		return MaxElements;
	}

	inline int32 GetSlack()
	{
		return MaxElements - NumElements;
	}

	inline bool IsValid()
	{
		return Data != nullptr;
	}

	inline bool IsValidIndex(int32 Index)
	{
		return Index >= 0 && Index < NumElements;
	}

	inline void ResetNum()
	{
		NumElements = 0;
	}
};
)";

	BasicHpp << R"(
class FString : public TArray<wchar_t>
{
public:
	inline FString() = default;

	using TArray::TArray;

	inline FString(const wchar_t* WChar)
	{
		MaxElements = NumElements = *WChar ? std::wcslen(WChar) + 1 : 0;

		if (NumElements)
		{
			Data = const_cast<wchar_t*>(WChar);
		}
	}

	inline FString operator=(const wchar_t*&& Other)
	{
		return FString(Other);
	}

	inline std::wstring ToWString()
	{
		if (IsValid())
		{
			return Data;
		}

		return L"";
	}

	inline std::string ToString()
	{
		if (IsValid())
		{
			std::wstring WData(Data);
			return std::string(WData.begin(), WData.end());
		}

		return "";
	}
};
)";
	// End class 'TArray/FString'



	
	/*
	if (Off::InSDK::AppendNameToString == 0x0 && !Settings::Internal::bUseNamePool)
	{
		PredefinedStruct FNameEntry = PredefinedStruct{
			.UniqueName = "FNameEntry", .Size = FNameEntryHeaderSize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};


		MemberBuilder NameEntryMembers;
		NameEntryMembers.Add("\tint32 NameIndex;\n", Off::FNameEntry::NameArray::IndexOffset, sizeof(int32));
		NameEntryMembers.Add(
			R"(
	union
	{
		char    AnsiName[1024];
		wchar_t WideName[1024];
	};
)", Off::FNameEntry::NameArray::StringOffset, 0x0);

		BasicHpp << std::format(R"(
class FNameEntry
{{
public:
	static constexpr uint32 NameWideMask = 0x1;
	static constexpr uint32 NameIndexShiftCount = 0x1;

public:
{}

public:
	inline bool IsWide() const
	{{
		return (NameIndex & NameWideMask);
	}}

	inline std::string GetString() const
	{{
		if (IsWide())
		{{
			std::wstring WideString(WideName);
			return std::string(WideString.begin(), WideString.end());
		}}

		return AnsiName;
	}}
}};
)", NameEntryMembers.GetMembers());

		BasicHpp << std::format(R"(
class TNameEntryArray
{{
public:
	
	static constexpr uint32 ChunkTableSize = {};
	static constexpr uint32 NumElementsPerChunk = 0x4000;

public:
	FNameEntry** Chunks[ChunkTableSize];
	int32 NumElements;
	int32 NumChunks;

public:
	inline bool IsValidIndex(int32 Index, int32 ChunkIdx, int32 InChunkIdx) const
	{{
		return return Index >= 0 && Index < NumElements;
	}}

	inline FNameEntry* GetEntryByIndex(int32 Index) const
	{{
		const int32 ChunkIdx = Index / NumElementsPerChunk;
		const int32 InChunk  = Index % NumElementsPerChunk;

		if (!IsValidIndex(Index, ChunkIdx, InChunk))
			return nullptr;

		return Chunks[ChunkIdx][InChunk];
	}}
}};
)", Off::NameArray::NumElements / 0x8);
	}
	else*/if (Off::InSDK::AppendNameToString == 0x0 && Settings::Internal::bUseNamePool)
	{
		/* struct FNumberedData */
		const int32 FNumberedDataSize = Settings::Internal::bUseCasePreservingName ? 0xA : 0x8;

		PredefinedStruct FNumberedData = PredefinedStruct{
			.UniqueName = "FNumberedData", .Size = FNumberedDataSize, .Alignment = 0x1, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		const int32 FNumberedDataInitialOffset = Settings::Internal::bUseCasePreservingName ? 0x2 : 0x0;

		FNumberedData.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint8", .Name = "Id", .Offset = FNumberedDataInitialOffset, .Size = 0x01, .ArrayDim = 0x4, .Alignment = 0x1,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0x0, .BitCount = 1
			},
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint8", .Name = "Number", .Offset = FNumberedDataInitialOffset + 0x4, .Size = 0x01, .ArrayDim = 0x4, .Alignment = 0x1,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		};

		FNumberedData.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "int32", .NameWithParams = "GetTypedId()", .Body = 
R"({
	return reinterpret_cast<int32>(Id);
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "uint32", .NameWithParams = "GetNumber()", .Body =
R"({
	return reinterpret_cast<uint32>(Number);
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};


		/* struct FNameEntryHeader */
		const int32 FNameEntryHeaderSize = Off::FNameEntry::NamePool::StringOffset - Off::FNameEntry::NamePool::HeaderOffset;

		PredefinedStruct FNameEntryHeader = PredefinedStruct{
			.UniqueName = "FNameEntryHeader", .Size = FNameEntryHeaderSize, .Alignment = 0x2, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		const uint8 LenBitCount = Settings::Internal::bUseCasePreservingName ? 15 : 10;
		const uint8 LenBitOffset = Settings::Internal::bUseCasePreservingName ? 1 : 6;

		FNameEntryHeader.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint16", .Name = "bIsWide", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = 0x0, .BitCount = 1
			},
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint16", .Name = "Len", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = LenBitOffset, .BitCount = LenBitCount
			}
		};


		/* struct FStringData */
		PredefinedStruct FStringData = PredefinedStruct{
			.UniqueName = "FStringData", .Size = 0x800, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = true, .Super = nullptr
		};

		FStringData.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "char", .Name = "AnsiName", .Offset = 0x00, .Size = 0x01, .ArrayDim = 0x400, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "wchar_t", .Name = "WideName", .Offset = 0x08, .Size = 0x02, .ArrayDim = 0x400, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		if (Settings::Internal::bUseUoutlineNumberName)
		{
			FStringData.Properties.push_back(
				PredefinedMember{
					.Comment = "NOT AUTO-GENERATED PROPERTY",
					.Type = "FNumberedData", .Name = "AnsiName", .Offset = 0x10, .Size = FNumberedDataSize, .ArrayDim = 0x400, .Alignment = 0x1,
					.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
				}
			);
		}


		/* struct FNameEntry */
		PredefinedStruct FNameEntry = PredefinedStruct{
			.UniqueName = "FNameEntry", .Size = Off::FNameEntry::NamePool::StringOffset + 0x08, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		FNameEntry.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "struct FNameEntryHeader", .Name = "Header", .Offset = 0x0, .Size = FNameEntryHeaderSize, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "struct FStringData", .Name = "Name", .Offset = FNameEntryHeaderSize, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		FNameEntry.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "bool", .NameWithParams = "IsWide()", .Body =
R"({
	return Header.bIsWide;
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "std::string", .NameWithParams = "GetString()", .Body =
R"({
	if (IsWide())
	{
		std::wstring WideString(Name.WideName, Header.Len);
		return std::string(WideString.begin(), WideString.end());
	}

	return std::string(Name.AnsiName, Header.Len);
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		/* class FNamePool */
		PredefinedStruct FNamePool = PredefinedStruct{
			.UniqueName = "FNamePool", .Size = Off::FNameEntry::NamePool::StringOffset + 0x08, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		FNamePool.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameBlockOffsetBits", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:04X}", Off::InSDK::FNamePoolBlockOffsetBits)
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameBlockOffsets", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "1 << FNameBlockOffsetBits"
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameEntryStride", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:04X}", Off::InSDK::FNameEntryStride)
			},

			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint32", .Name = "CurrentBlock", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint32", .Name = "CurrentByteCursor", .Offset = 0x4, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint8*", .Name = "Blocks", .Offset = FNameEntryHeaderSize, .Size = 0x8, .ArrayDim = 0x2000, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		FNamePool.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "bool", .NameWithParams = "IsValidIndex(int32 Index, int32 ChunkIdx, int32 InChunkIdx)", .Body =
R"({
	return ChunkIdx <= CurrentBlock && !(ChunkIdx == CurrentBlock && InChunkIdx > CurrentByteCursor);
}
)",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "FNameEntry*", .NameWithParams = "GetEntryByIndex(int32 Index)", .Body =
R"({
	const int32 ChunkIdx = Index >> FNameBlockOffsetBits;
	const int32 InChunk = (Index & (FNameBlockOffsets - 1));

	if (!IsValidIndex(Index, ChunkIdx, InChunk))
		return nullptr;

	return reinterpret_cast<FNameEntry*>(Blocks[ChunkIdx] + (InChunk * FNameEntryStride));
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		GenerateStruct(&FNumberedData, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&FNameEntryHeader, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&FStringData, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&FNameEntry, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&FNamePool, BasicHpp, BasicCpp, BasicHpp);
	}




	PredefinedStruct FTestStruct = PredefinedStruct{
		.UniqueName = "FTestStruct", .Size = 0x8, .Alignment = 0x2, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
	};
	
	FTestStruct.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint16", .Name = "bIsWide", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x1, .Alignment = 0x2,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = 0x0, .BitCount = 1
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint16", .Name = "This4BitSize", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x1, .Alignment = 0x2,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = 0x1, .BitCount = 4
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint16", .Name = "SomeBigerField", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x1, .Alignment = 0x2,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = 0x8, .BitCount = 6
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "thisisDefaultbeifeld", .Offset = 0x2, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = true, .BitIndex = 0x0, .BitCount = 1
		},
	};
	GenerateStruct(&FTestStruct, BasicHpp, BasicCpp, BasicHpp);

	/* Write Predefined Structs into Basic.hpp */
	for (const PredefinedStruct& Predefined : PredefinedStructs)
	{
		GenerateStruct(&Predefined, BasicHpp, BasicCpp, BasicHpp);
	}

	WriteFileEnd(BasicHpp, EFileType::BasicHpp);
	WriteFileEnd(BasicCpp, EFileType::BasicCpp);
}
