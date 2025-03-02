
#include <vector>
#include <array>

#include "Unreal/ObjectArray.h"
#include "Generators/CppGenerator.h"
#include "Wrappers/MemberWrappers.h"
#include "Managers/MemberManager.h"

#include "../Settings.h"

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

std::string CppGenerator::MakeMemberStringWithoutName(const std::string& Type)
{
	return '\t' + Type + ";\n";
}

std::string CppGenerator::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	return MakeMemberString("uint8", std::format("Pad_{:X}[0x{:X}]", Offset, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateBitPadding(uint8 UnderlayingSizeBytes, const uint8 PrevBitPropertyEndBit, const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	return MakeMemberString(GetTypeFromSize(UnderlayingSizeBytes), std::format("BitPad_{:X}_{:X} : {:X}", Offset, PrevBitPropertyEndBit, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, UnderlayingSizeBytes, std::move(Reason)));
}

std::string CppGenerator::GenerateMembers(const StructWrapper& Struct, const MemberManager& Members, int32 SuperSize, int32 SuperLastMemberEnd, int32 SuperAlign, int32 PackageIndex)
{
	constexpr uint64 EstimatedCharactersPerLine = 0x80;

	const bool bIsUnion = Struct.IsUnion();

	std::string OutMembers;
	OutMembers.reserve(Members.GetNumMembers() * EstimatedCharactersPerLine);

	bool bEncounteredZeroSizedVariable = false;
	bool bEncounteredStaticVariable = false;
	bool bAddedSpaceZeroSized = false;
	bool bAddedSpaceStatic = false;

	auto AddSpaceBetweenSticAndNormalMembers = [&](const PropertyWrapper& Member)
	{
		if (!bAddedSpaceStatic && bEncounteredZeroSizedVariable && !Member.IsZeroSizedMember()) [[unlikely]]
		{
			OutMembers += '\n';
			bAddedSpaceZeroSized = true;
		}

		if (!bAddedSpaceStatic && bEncounteredStaticVariable && !Member.IsStatic()) [[unlikely]]
		{
			OutMembers += '\n';
			bAddedSpaceStatic = true;
		}

		bEncounteredZeroSizedVariable = Member.IsZeroSizedMember();
		bEncounteredStaticVariable = Member.IsStatic() && !Member.IsZeroSizedMember();
	};

	bool bLastPropertyWasBitField = false;

	int32 PrevPropertyEnd = SuperSize;
	int32 PrevBitPropertyEnd = 0;
	int32 PrevBitPropertyEndBit = 1;

	uint8 PrevBitPropertySize = 0x1;
	int32 PrevBitPropertyOffset = 0x0;
	uint64 PrevNumBitsInUnderlayingType = 0x8;

	const int32 SuperTrailingPaddingSize = SuperSize - SuperLastMemberEnd;
	bool bIsFirstSizedMember = true;

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		AddSpaceBetweenSticAndNormalMembers(Member);

		const int32 MemberOffset = Member.GetOffset();
		const int32 MemberSize = Member.GetSize();

		const int32 CurrentPropertyEnd = MemberOffset + MemberSize;

		std::string Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, Member.GetFlagsOrCustomComment());


		const bool bIsBitField = Member.IsBitField();

		/* Padding between two bitfields at different byte-offsets */
		if (CurrentPropertyEnd > PrevPropertyEnd && bLastPropertyWasBitField && bIsBitField && PrevBitPropertyEndBit < PrevNumBitsInUnderlayingType && !bIsUnion)
		{
			OutMembers += GenerateBitPadding(PrevBitPropertySize, PrevBitPropertyEndBit, PrevBitPropertyOffset, PrevNumBitsInUnderlayingType - PrevBitPropertyEndBit, "Fixing Bit-Field Size For New Byte [ Dumper-7 ]");
			PrevBitPropertyEndBit = 0;
		}

		if (MemberOffset > PrevPropertyEnd && !bIsUnion)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, MemberOffset - PrevPropertyEnd, "Fixing Size After Last Property [ Dumper-7 ]");

		bIsFirstSizedMember = Member.IsZeroSizedMember() || Member.IsStatic();

		if (bIsBitField)
		{
			uint8 BitFieldIndex = Member.GetBitIndex();
			uint8 BitSize = Member.GetBitCount();

			if (CurrentPropertyEnd > PrevPropertyEnd)
				PrevBitPropertyEndBit = 0x0;

			std::string BitfieldInfoComment = std::format("BitIndex: 0x{:02X}, PropSize: 0x{:04X} ({})", BitFieldIndex, MemberSize, Member.GetFlagsOrCustomComment());
			Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, BitfieldInfoComment);

			if (PrevBitPropertyEnd < MemberOffset)
				PrevBitPropertyEndBit = 0;

			if (PrevBitPropertyEndBit < BitFieldIndex && !bIsUnion)
				OutMembers += GenerateBitPadding(MemberSize, PrevBitPropertyEndBit, MemberOffset, BitFieldIndex - PrevBitPropertyEndBit, "Fixing Bit-Field Size Between Bits [ Dumper-7 ]");

			PrevBitPropertyEndBit = BitFieldIndex + BitSize;
			PrevBitPropertyEnd = MemberOffset  + MemberSize;

			PrevBitPropertySize = MemberSize;
			PrevBitPropertyOffset = MemberOffset;

			PrevNumBitsInUnderlayingType = (MemberSize * 0x8);
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

		const bool bAllowForConstPtrMembers = Struct.IsFunction();

		/* using directives */
		if (Member.IsZeroSizedMember()) [[unlikely]]
		{
			OutMembers += MakeMemberStringWithoutName(GetMemberTypeString(Member, PackageIndex, bAllowForConstPtrMembers));
		}
		else [[likely]]
		{
			OutMembers += MakeMemberString(GetMemberTypeString(Member, PackageIndex, bAllowForConstPtrMembers), MemberName, std::move(Comment));
		}
	}

	const int32 MissingByteCount = Struct.GetUnalignedSize() - PrevPropertyEnd;

	if (MissingByteCount > 0x0 /* >=Struct.GetAlignment()*/)
		OutMembers += GenerateBytePadding(PrevPropertyEnd, MissingByteCount, "Fixing Struct Size After Last Property [ Dumper-7 ]");

	return OutMembers;
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
		if (!Param.HasPropertyFlags(EPropertyFlags::Parm))
			continue;

		std::string Type = GetMemberTypeString(Param);

		const bool bIsConst = Param.HasPropertyFlags(EPropertyFlags::ConstParm);

		ParamInfo PInfo;

		const bool bIsRef = Param.HasPropertyFlags(EPropertyFlags::ReferenceParm);
		const bool bIsOut = bIsRef || Param.HasPropertyFlags(EPropertyFlags::OutParm);
		const bool bIsRet = Param.IsReturnParam();

		if (bIsConst && (!bIsOut || bIsRef || bIsRet))
			Type = "const " + Type;

		if (Param.IsReturnParam())
		{
			RetFuncInfo.RetType = Type;
			RetFuncInfo.bIsReturningVoid = false;

			PInfo.PropFlags = Param.GetPropertyFlags();
			PInfo.bIsConst = false;
			PInfo.Name = Param.GetName();
			PInfo.Type = Type;
			PInfo.bIsRetParam = true;
			RetFuncInfo.UnrealFuncParams.push_back(PInfo);
			continue;
		}

		const bool bIsMoveType = Param.IsType(EClassCastFlags::StructProperty | EClassCastFlags::ArrayProperty | EClassCastFlags::StrProperty | EClassCastFlags::TextProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty);

		if (bIsOut)
			Type += bIsRef ? '&' : '*';

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
		PInfo.bIsConst = bIsConst;
		PInfo.PropFlags = Param.GetPropertyFlags();
		PInfo.Name = ParamName;
		PInfo.Type = Type;
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
	namespace CppSettings = Settings::CppGenerator;

	std::string InHeaderFunctionText;

	FunctionInfo FuncInfo = GenerateFunctionInfo(Func);

	const bool bHasInlineBody = Func.HasInlineBody();
	const std::string TemplateText = (bHasInlineBody && Func.HasCustomTemplateText() ? (Func.GetPredefFunctionCustomTemplateText() + "\n\t") : "");

	const bool bIsConstFunc = Func.IsConst() && !Func.IsStatic();

	// Function declaration and inline-body generation
	InHeaderFunctionText += std::format("\t{}{}{} {}{}", TemplateText, (Func.IsStatic() ? "static " : ""), FuncInfo.RetType, FuncInfo.FuncNameWithParams, bIsConstFunc ? " const" : "");
	InHeaderFunctionText += (bHasInlineBody ? ("\n\t" + Func.GetPredefFunctionInlineBody()) : ";") + "\n";

	if (bHasInlineBody)
		return InHeaderFunctionText;

	if (Func.IsPredefined())
	{
		std::string CustomComment = Func.GetPredefFunctionCustomComment();

		FunctionFile << std::format(R"(
// Predefined Function
{}
{} {}::{}{}
{}

)"
, !CustomComment.empty() ? ("// " + CustomComment + '\n') : ""
, Func.GetPredefFuncReturnType()
, StructName
, Func.GetPredefFuncNameWithParamsForCppFile()
, bIsConstFunc ? " const" : ""
, Func.GetPredefFunctionBody());

		return InHeaderFunctionText;
	}

	std::string ParamStructName = Func.GetParamStructName();

	// Parameter struct generation for unreal-functions
	if (!Func.IsPredefined() && Func.GetParamStructSize() > 0x0)
		GenerateStruct(Func.AsStruct(), ParamFile, FunctionFile, ParamFile, -1, ParamStructName);


	std::string ParamVarCreationString = std::format(R"(
	{}{} Parms{{}};
)", CppSettings::ParamNamespaceName ? std::format("{}::", CppSettings::ParamNamespaceName) : "", ParamStructName);

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
			OutPtrAssignments += !PInfo.bIsMoveParam ? std::format(R"(

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

		if (PInfo.bIsOutRef && !PInfo.bIsConst)
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

	static auto PrefixQuotsWithBackslash = [](std::string&& Str) -> std::string
	{
		for (int i = 0; i < Str.size(); i++)
		{
			if (Str[i] == '"')
			{
				Str.insert(i, "\\");
				i++;
			}
		}

		return Str;
	};

	std::string FixedOuterName = PrefixQuotsWithBackslash(UnrealFunc.GetOuter().GetName());
	std::string FixedFunctionName = PrefixQuotsWithBackslash(UnrealFunc.GetName());

	// Function implementation generation
	std::string FunctionImplementation = std::format(R"(
// {}
// ({})
{}
{} {}::{}{}
{{
	static class UFunction* Func = nullptr;

	if (Func == nullptr)
		Func = {}->GetFunction("{}", "{}");
{}{}{}
	{}ProcessEvent(Func, {});{}{}{}{}
}}

)", UnrealFunc.GetFullName()
, StringifyFunctionFlags(FuncInfo.FuncFlags)
, bHasParams ? ParamDescriptionCommentString : ""
, FuncInfo.RetType
, StructName
, FuncInfo.FuncNameWithParams
, bIsConstFunc ? " const" : ""
, Func.IsStatic() ? "StaticClass()" : "Class"
, FixedOuterName
, FixedFunctionName
, bHasParams ? ParamVarCreationString : ""
, bHasParamsToInit ? ParamAssignments : ""
, bIsNativeFunc ? StoreFunctionFlagsString : ""
, Func.IsStatic() ? "GetDefaultObj()->" : "UObject::"
, bHasParams ? "&Parms" : "nullptr"
, bIsNativeFunc ? RestoreFunctionFlagsString : ""
, bHasOutRefParamsToInit ? OutRefAssignments : ""
, bHasOutPtrParamsToInit ? OutPtrAssignments : ""
, !FuncInfo.bIsReturningVoid ? ReturnValueString : "");

	FunctionFile << FunctionImplementation;

	return InHeaderFunctionText;
}

std::string CppGenerator::GenerateFunctions(const StructWrapper& Struct, const MemberManager& Members, const std::string& StructName, StreamType& FunctionFile, StreamType& ParamFile)
{
	namespace CppSettings = Settings::CppGenerator;

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
		/* The function is no callable function, but instead just the signature of a TDelegate or TMulticastInlineDelegate */
		if (Func.GetFunctionFlags() & EFunctionFlags::Delegate)
			continue;

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

	/* Skip predefined classes, all structs and classes which don't inherit from UObject (very rare). */
	if (!Struct.IsUnrealStruct() || !Struct.IsClass() || !Struct.GetSuper().IsValid())
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
	

	static UEClass BPGeneratedClass = nullptr;
	
	if (BPGeneratedClass == nullptr)
		BPGeneratedClass = ObjectArray::FindClassFast("BlueprintGeneratedClass");


	const char* StaticClassImplFunctionName = "StaticClassImpl";

	std::string NonFullName;

	const bool bIsBPStaticClass = Struct.IsAClassWithType(BPGeneratedClass);

	/* BPGenerated classes are loaded/unloaded dynamically, so a static pointer to the UClass will eventually be invalidated */
	if (bIsBPStaticClass)
	{
		StaticClassImplFunctionName = "StaticBPGeneratedClassImpl";

		if (!bIsNameUnique)
			NonFullName = CppSettings::XORString ? std::format("{}(\"{}\")", CppSettings::XORString, Struct.GetRawName()) : std::format("\"{}\"", Struct.GetRawName());
	}
	

	/* Use the default implementation of 'StaticClass' */
	StaticClass.Body = std::format(
			R"({{
	return {}<{}{}{}>();
}})", StaticClassImplFunctionName, NameText, (!bIsNameUnique ? ", true" : ""), (bIsBPStaticClass && !bIsNameUnique ? ", " + NonFullName : ""));

	/* Set class-specific parts of 'GetDefaultObj' */
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

void CppGenerator::GenerateStruct(const StructWrapper& Struct, StreamType& StructFile, StreamType& FunctionFile, StreamType& ParamFile, int32 PackageIndex, const std::string& StructNameOverride)
{
	if (!Struct.IsValid())
		return;

	std::string UniqueName = StructNameOverride.empty() ? GetStructPrefixedName(Struct) : StructNameOverride;
	std::string UniqueSuperName;

	const int32 StructSize = Struct.GetSize();
	int32 SuperSize = 0x0;
	int32 UnalignedSuperSize = 0x0;
	int32 SuperAlignment = 0x0;
	int32 SuperLastMemberEnd = 0x0;
	bool bIsReusingTrailingPaddingFromSuper = false;

	StructWrapper Super = Struct.GetSuper();

	const bool bHasValidSuper = Super.IsValid() && !Struct.IsFunction();

	/* Ignore UFunctions with a valid Super field, parameter structs are not supposed inherit from eachother. */
	if (bHasValidSuper)
	{
		UniqueSuperName = GetStructPrefixedName(Super);
		SuperSize = Super.GetSize();
		UnalignedSuperSize = Super.GetUnalignedSize();
		SuperAlignment = Super.GetAlignment();
		SuperLastMemberEnd = Super.GetLastMemberEnd();

		bIsReusingTrailingPaddingFromSuper = Super.HasReusedTrailingPadding();

		if (Super.IsCyclicWithPackage(PackageIndex)) [[unlikely]]
			UniqueSuperName = GetCycleFixupType(Super, true);
	}

	const int32 StructSizeWithoutSuper = StructSize - SuperSize;

	const bool bIsClass = Struct.IsClass();
	const bool bIsUnion = Struct.IsUnion();

	const bool bHasReusedTrailingPadding = Struct.HasReusedTrailingPadding();


	StructFile << std::format(R"(
// {}
// 0x{:04X} (0x{:04X} - 0x{:04X})
{}{}{} {}{}{}{}
{{
)", Struct.GetFullName()
  , StructSizeWithoutSuper
  , StructSize
  , SuperSize
  , bHasReusedTrailingPadding ? "#pragma pack(push, 0x1)\n" : ""
  , Struct.HasCustomTemplateText() ? (Struct.GetCustomTemplateText() + "\n") : ""
  , bIsClass ? "class" : (bIsUnion ? "union" : "struct")
  , Struct.ShouldUseExplicitAlignment() || bHasReusedTrailingPadding ? std::format("alignas(0x{:02X}) ", Struct.GetAlignment()) : ""
  , UniqueName
  , Struct.IsFinal() ? " final" : ""
  , bHasValidSuper ? (" : public " + UniqueSuperName) : "");

	MemberManager Members = Struct.GetMembers();

	const bool bHasStaticClass = (bIsClass && Struct.IsUnrealStruct());

	const bool bHasMembers = Members.HasMembers() || (StructSizeWithoutSuper >= Struct.GetAlignment());
	const bool bHasFunctions = (Members.HasFunctions() && !Struct.IsFunction()) || bHasStaticClass;

	if (bHasMembers || bHasFunctions)
		StructFile << "public:\n";

	if (bHasMembers)
	{
		StructFile << GenerateMembers(Struct, Members, bIsReusingTrailingPaddingFromSuper ? UnalignedSuperSize : SuperSize, SuperLastMemberEnd, SuperAlignment, PackageIndex);

		if (bHasFunctions)
			StructFile << "\npublic:\n";
	}

	if (bHasFunctions)
		StructFile << GenerateFunctions(Struct, Members, UniqueName, FunctionFile, ParamFile);

	StructFile << "};\n";

	if (bHasReusedTrailingPadding)
		StructFile << "#pragma pack(pop)\n";

	if constexpr (Settings::Debug::bGenerateInlineAssertionsForStructSize)
	{
		if (Struct.HasCustomTemplateText())
			return;

		std::string UniquePrefixedName = StructNameOverride.empty() ? GetStructPrefixedName(Struct) : StructNameOverride;

		const int32 StructSize = Struct.GetSize();

		// Alignment assertions
		StructFile << std::format("static_assert(alignof({}) == 0x{:06X}, \"Wrong alignment on {}\");\n", UniquePrefixedName, Struct.GetAlignment(), UniquePrefixedName);

		// Size assertions
		StructFile << std::format("static_assert(sizeof({}) == 0x{:06X}, \"Wrong size on {}\");\n", UniquePrefixedName, (StructSize > 0x0 ? StructSize : 0x1), UniquePrefixedName);
	}


	if constexpr (Settings::Debug::bGenerateInlineAssertionsForStructMembers)
	{
		std::string UniquePrefixedName = StructNameOverride.empty() ? GetStructPrefixedName(Struct) : StructNameOverride;

		for (const PropertyWrapper& Member : Members.IterateMembers())
		{
			if (Member.IsBitField() || Member.IsZeroSizedMember() || Member.IsStatic())
				continue;

			std::string MemberName = Member.GetName();

			StructFile << std::format("static_assert(offsetof({0}, {1}) == 0x{2:06X}, \"Member '{0}::{1}' has a wrong offset!\");\n", UniquePrefixedName, Member.GetName(), Member.GetOffset());
		}
	}
}

void CppGenerator::GenerateEnum(const EnumWrapper& Enum, StreamType& StructFile)
{
	if (!Enum.IsValid())
		return;

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
  , GetEnumUnderlayingType(Enum)
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

std::string CppGenerator::GetEnumUnderlayingType(const EnumWrapper& Enum)
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

	return Enum.GetUnderlyingTypeSize() <= 0x8 ? UnderlayingTypesBySize[static_cast<size_t>(Enum.GetUnderlyingTypeSize()) - 1] : "uint8";
}

std::string CppGenerator::GetCycleFixupType(const StructWrapper& Struct, bool bIsForInheritance)
{
	static int32 UObjectSize = 0x0;
	static int32 AActorSize = 0x0;

	if (UObjectSize == 0x0 || AActorSize == 0x0)
	{
		UObjectSize = StructWrapper(ObjectArray::FindClassFast("Object")).GetSize();
		AActorSize = StructWrapper(ObjectArray::FindClassFast("Actor")).GetSize();
	}

	/* Predefined structs can not be cyclic, unless you did something horribly wrong when defining the predefined struct! */
	if (!Struct.IsUnrealStruct())
		return "Invalid+Fixup+Type";

	/* ToDo: find out if we need to differ between using Unaligned-/Aligned-Size on Inheritance/Members */
	const int32 OwnSize = Struct.GetUnalignedSize();
	const int32 Align = Struct.GetAlignment();

	std::string Name = GetStructPrefixedName(Struct);

	/* For structs the fixup is always the same, so we handle them before classes */
	if (!Struct.IsClass())
		return std::format("TStructCycleFixup<struct {}, 0x{:04X}, 0x{:02X}>", Name, OwnSize, Align);

	const bool bIsActor = Struct.GetUnrealStruct().IsA(EClassCastFlags::Actor);

	if (bIsActor)
		return std::format("TActorBasedCycleFixup<class {}, 0x{:04X}, 0x{:02X}>", Name, (OwnSize - AActorSize), Align);

	return std::format("TObjectBasedCycleFixup<class {}, 0x{:04X}, 0x{:02X}>", Name, (OwnSize - UObjectSize), Align);
}

std::string CppGenerator::GetMemberTypeString(const PropertyWrapper& MemberWrapper, int32 PackageIndex, bool bAllowForConstPtrMembers)
{
	if (!MemberWrapper.IsUnrealProperty())
	{
		if (MemberWrapper.IsStatic() && !MemberWrapper.IsZeroSizedMember())
			return "static " + MemberWrapper.GetType();

		return MemberWrapper.GetType();
	}

	return GetMemberTypeString(MemberWrapper.GetUnrealProperty(), PackageIndex, bAllowForConstPtrMembers);
}

std::string CppGenerator::GetMemberTypeString(UEProperty Member, int32 PackageIndex, bool bAllowForConstPtrMembers)
{
	static auto IsMemberPtr = [](UEProperty Mem) -> bool
	{
		if (Mem.IsA(EClassCastFlags::ClassProperty))
			return !Mem.HasPropertyFlags(EPropertyFlags::UObjectWrapper);

		return Mem.IsA(EClassCastFlags::ObjectProperty);
	};

	if (bAllowForConstPtrMembers && Member.HasPropertyFlags(EPropertyFlags::ConstParm) && IsMemberPtr(Member))
		return "const " + GetMemberTypeStringWithoutConst(Member, PackageIndex);

	return GetMemberTypeStringWithoutConst(Member, PackageIndex);
}

std::string CppGenerator::GetMemberTypeStringWithoutConst(UEProperty Member, int32 PackageIndex)
{
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
		const StructWrapper& UnderlayingStruct = Member.Cast<UEStructProperty>().GetUnderlayingStruct();

		if (UnderlayingStruct.IsCyclicWithPackage(PackageIndex)) [[unlikely]]
			return std::format("{}", GetCycleFixupType(UnderlayingStruct, false));

		return std::format("struct {}", GetStructPrefixedName(UnderlayingStruct));
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return std::format("TArray<{}>", GetMemberTypeStringWithoutConst(Member.Cast<UEArrayProperty>().GetInnerProperty(), PackageIndex));
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

		return std::format("TMap<{}, {}>", GetMemberTypeStringWithoutConst(MemberAsMapProperty.GetKeyProperty(), PackageIndex), GetMemberTypeStringWithoutConst(MemberAsMapProperty.GetValueProperty(), PackageIndex));
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return std::format("TSet<{}>", GetMemberTypeStringWithoutConst(Member.Cast<UESetProperty>().GetElementProperty(), PackageIndex));
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		if (UEEnum Enum = Member.Cast<UEEnumProperty>().GetEnum())
			return GetEnumPrefixedName(Enum);

		return GetMemberTypeStringWithoutConst(Member.Cast<UEEnumProperty>().GetUnderlayingProperty(), PackageIndex);
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEInterfaceProperty>().GetPropertyClass())
			return std::format("TScriptInterface<class {}>", GetStructPrefixedName(PropertyClass));

		return "TScriptInterface<class IInterface>";
	}
	else if (Flags & EClassCastFlags::DelegateProperty)
	{
		if (UEFunction SignatureFunc = Member.Cast<UEDelegateProperty>().GetSignatureFunction()) [[likely]]
			return std::format("TDelegate<{}>", GetFunctionSignature(SignatureFunc));

		return "TDelegate<void()>";
	}
	else if (Flags & EClassCastFlags::MulticastInlineDelegateProperty)
	{
		if (UEFunction SignatureFunc = Member.Cast<UEMulticastInlineDelegateProperty>().GetSignatureFunction()) [[likely]]
			return std::format("TMulticastInlineDelegate<{}>", GetFunctionSignature(SignatureFunc));

		return "TMulticastInlineDelegate<void()>";
	}
	else if (Flags & EClassCastFlags::FieldPathProperty)
	{
		return std::format("TFieldPath<class {}>", Member.Cast<UEFieldPathProperty>().GetFielClass().GetCppName());
	}
	else if (Flags & EClassCastFlags::OptionalProperty)
	{
		UEProperty ValueProperty = Member.Cast<UEOptionalProperty>().GetValueProperty();

		/* Check if there is an additional 'bool' flag in the TOptional to check if the value is set */
		if (Member.GetSize() > ValueProperty.GetSize()) [[likely]]
			return std::format("TOptional<{}>", GetMemberTypeStringWithoutConst(ValueProperty, PackageIndex));

		return std::format("TOptional<{}, true>", GetMemberTypeStringWithoutConst(ValueProperty, PackageIndex));
	}
	else
	{
		/* When changing this also change 'GetUnknownProperties()' */
		return (Class ? Class.GetCppName() : FieldClass.GetCppName()) + "_";
	}
}

std::string CppGenerator::GetFunctionSignature(UEFunction Func)
{
	std::string RetType = "void";

	std::string OutParameters;

	bool bIsFirstParam = true;

	std::vector<UEProperty> Params = Func.GetProperties();
	std::sort(Params.begin(), Params.end(), CompareUnrealProperties);

	for (UEProperty Param : Params)
	{
		std::string Type = GetMemberTypeString(Param);

		const bool bIsConst = Param.HasPropertyFlags(EPropertyFlags::ConstParm);

		if (Param.HasPropertyFlags(EPropertyFlags::ReturnParm))
		{
			if (bIsConst)
				RetType = "const " + Type;

			continue;
		}

		const bool bIsRef = Param.HasPropertyFlags(EPropertyFlags::ReferenceParm);
		const bool bIsOut = bIsRef || Param.HasPropertyFlags(EPropertyFlags::OutParm);
		const bool bIsMoveType = Param.IsType(EClassCastFlags::StructProperty | EClassCastFlags::ArrayProperty | EClassCastFlags::StrProperty | EClassCastFlags::MapProperty | EClassCastFlags::SetProperty);

		if (bIsConst && (!bIsOut || bIsRef))
			Type = "const " + Type;

		if (bIsOut)
			Type += bIsRef ? '&' : '*';

		if (!bIsOut && !bIsRef && bIsMoveType)
		{
			Type += "&";

			if (!bIsConst)
				Type = "const " + Type;
		}

		std::string ParamName = Param.GetValidName();

		if (!bIsFirstParam)
			OutParameters += ", ";

		OutParameters += Type + " " + ParamName;

		bIsFirstParam = false;
	}

	return RetType + "(" + OutParameters + ")";
}


std::unordered_map<std::string, UEProperty> CppGenerator::GetUnknownProperties()
{
	std::unordered_map<std::string, UEProperty> PropertiesWithNames;

	for (UEObject Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct))
			continue;

		for (UEProperty Prop : Obj.Cast<UEStruct>().GetProperties())
		{
			std::string TypeName = GetMemberTypeString(Prop);

			auto [Class, FieldClass] = Prop.GetClass();

			/* Relies on unknown names being post-fixed with an underscore by 'GetMemberTypeString()' */
			if (TypeName.back() == '_')
				PropertiesWithNames[TypeName] = Prop;
		}
	}

	return PropertiesWithNames;
}

void CppGenerator::GeneratePropertyFixupFile(StreamType& PropertyFixup)
{
	WriteFileHead(PropertyFixup, nullptr, EFileType::PropertyFixup, "PROPERTY-FIXUP");

	std::unordered_map<std::string, UEProperty> UnknownProperties = GetUnknownProperties();

	for (const auto& [Name, Property] : UnknownProperties)
	{
		PropertyFixup << std::format("\nclass alignas(0x{:02X}) {}\n{{\n\tunsigned __int8 Pad[0x{:X}];\n}};\n",Property.GetAlignment(), Name, Property.GetSize());
	}

	WriteFileEnd(PropertyFixup, EFileType::PropertyFixup);
}

void CppGenerator::GenerateEnumFwdDeclarations(StreamType& ClassOrStructFile, PackageInfoHandle Package, bool bIsClassFile)
{
	const std::vector<std::pair<int32, bool>>& FwdDeclarations = Package.GetEnumForwardDeclarations();

	for (const auto [EnumIndex, bIsForClassFile] : FwdDeclarations)
	{
		if (bIsForClassFile != bIsClassFile)
			continue;

		EnumWrapper Enum = EnumWrapper(ObjectArray::GetByIndex<UEEnum>(EnumIndex));

		ClassOrStructFile << std::format("enum class {} : {};\n", GetEnumPrefixedName(Enum), GetEnumUnderlayingType(Enum));
	}
}

void CppGenerator::GenerateNameCollisionsInl(StreamType& NameCollisionsFile)
{
	namespace CppSettings = Settings::CppGenerator;

	WriteFileHead(NameCollisionsFile, nullptr, EFileType::NameCollisionsInl, "FORWARD DECLARATIONS");


	const StructManager::OverrideMaptType& StructInfoMap = StructManager::GetStructInfos();
	const EnumManager::OverrideMaptType& EnumInfoMap = EnumManager::GetEnumInfos();

	std::unordered_map<int32 /* PackageIdx */, std::pair<std::string, int32>> PackagesAndForwardDeclarations;

	for (const auto& [Index, Info] : StructInfoMap)
	{
		if (StructManager::IsStructNameUnique(Info.Name))
			continue;

		UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(Index);

		if (Struct.IsA(EClassCastFlags::Function))
			continue;

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

		ForwardDeclarations += std::format("\tenum class {} : {};\n", Enum.GetEnumPrefixedName(), GetEnumUnderlayingType(Enum));
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

void CppGenerator::GenerateDebugAssertions(StreamType& AssertionStream)
{
	WriteFileHead(AssertionStream, nullptr, EFileType::DebugAssertions, "Debug assertions to verify member-offsets and struct-sizes");

	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		DependencyManager::OnVisitCallbackType GenerateStructAssertionsCallback = [&AssertionStream](int32 Index) -> void
		{
			StructWrapper Struct = ObjectArray::GetByIndex<UEStruct>(Index);

			std::string UniquePrefixedName = GetStructPrefixedName(Struct);

			AssertionStream << std::format("// {} {}\n", (Struct.IsClass() ? "class" : "struct"), UniquePrefixedName);

			// Alignment assertions
			AssertionStream << std::format("static_assert(alignof({}) == 0x{:06X});\n", UniquePrefixedName, Struct.GetAlignment());

			const int32 StructSize = Struct.GetSize();

			// Size assertions
			AssertionStream << std::format("static_assert(sizeof({}) == 0x{:06X});\n", UniquePrefixedName, (StructSize > 0x0 ? StructSize : 0x1));

			AssertionStream << "\n";

			// Member offset assertions
			MemberManager Members = Struct.GetMembers();

			for (const PropertyWrapper& Member : Members.IterateMembers())
			{
				if (Member.IsStatic() || Member.IsZeroSizedMember() || Member.IsBitField())
					continue;

				AssertionStream << std::format("static_assert(offsetof({}, {}) == 0x{:06X});\n", UniquePrefixedName, Member.GetName(), Member.GetOffset());
			}

			AssertionStream << "\n\n";
		};

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			Structs.VisitAllNodesWithCallback(GenerateStructAssertionsCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();

			Classes.VisitAllNodesWithCallback(GenerateStructAssertionsCallback);
		}
	}

	WriteFileEnd(AssertionStream, EFileType::DebugAssertions);
}

void CppGenerator::GenerateSDKHeader(StreamType& SdkHpp)
{
	WriteFileHead(SdkHpp, nullptr, EFileType::SdkHpp, "Includes the entire SDK, include files directly for faster compilation!");


	auto ForEachElementCallback = [&SdkHpp](const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct) -> void
	{
		PackageInfoHandle CurrentPackage = PackageManager::GetInfo(NewParams.RequiredPackage);

		const bool bHasClassesFile = CurrentPackage.HasClasses();
		const bool bHasStructsFile = (CurrentPackage.HasStructs() || CurrentPackage.HasEnums());

		if (bIsStruct && bHasStructsFile)
			SdkHpp << std::format("#include \"SDK/{}_structs.hpp\"\n", CurrentPackage.GetName());

		if (!bIsStruct && bHasClassesFile)
			SdkHpp << std::format("#include \"SDK/{}_classes.hpp\"\n", CurrentPackage.GetName());
	};

	PackageManager::IterateDependencies(ForEachElementCallback);


	WriteFileEnd(SdkHpp, EFileType::SdkHpp);
}

void CppGenerator::WriteFileHead(StreamType& File, PackageInfoHandle Package, EFileType Type, const std::string& CustomFileComment, const std::string& CustomIncludes)
{
	namespace CppSettings = Settings::CppGenerator;

	/* Write the utf8 BOM to indicate that this is a utf8 encoded file. */
	File << "\xEF\xBB\xBF";

	File << R"(#pragma once

/*
* SDK generated by Dumper-7
*
* https://github.com/Encryqed/Dumper-7
*/
)";

	if (Type == EFileType::SdkHpp)
		File << std::format("\n// {}\n// {}\n", Settings::Generator::GameName, Settings::Generator::GameVersion);
	

	File << std::format("\n// {}\n\n", Package.IsValidHandle() ? std::format("Package: {}", Package.GetName()) : CustomFileComment);


	if (!CustomIncludes.empty())
		File << CustomIncludes + "\n";

	if (Type != EFileType::BasicHpp && Type != EFileType::NameCollisionsInl && Type != EFileType::PropertyFixup && Type != EFileType::SdkHpp && Type != EFileType::DebugAssertions && Type != EFileType::UnrealContainers && Type != EFileType::UnicodeLib)
		File << "#include \"Basic.hpp\"\n";

	if (Type == EFileType::SdkHpp)
		File << "#include \"SDK/Basic.hpp\"\n";

	if (Type == EFileType::DebugAssertions)
		File << "#include \"SDK.hpp\"\n";

	if (Type == EFileType::BasicHpp)
	{
		File << "#include \"../PropertyFixup.hpp\"\n";
		File << "#include \"../UnrealContainers.hpp\"\n";
	}

	if (Type == EFileType::BasicCpp)
	{
		File << "\n#include \"CoreUObject_classes.hpp\"";
		File << "\n#include \"CoreUObject_structs.hpp\"\n";
	}

	if (Type == EFileType::Functions && (Package.HasClasses() || Package.HasParameterStructs()))
	{
		std::string PackageName = Package.GetName();

		File << "\n";

		if (Package.HasClasses())
			File << std::format("#include \"{}_classes.hpp\"\n", PackageName);

		if (Package.HasParameterStructs())
			File << std::format("#include \"{}_parameters.hpp\"\n", PackageName);

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
				File << std::format("#include \"{}_structs.hpp\"\n", DependencyName);

			if (Requirements.bShouldIncludeClasses)
				File << std::format("#include \"{}_classes.hpp\"\n", DependencyName);
		}

		if (bAddNewLine)
			File << "\n";
	}

	if (Type == EFileType::SdkHpp || Type == EFileType::NameCollisionsInl || Type == EFileType::UnrealContainers || Type == EFileType::UnicodeLib)
		return; /* No namespace or packing in SDK.hpp or NameCollisions.inl */


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
	namespace CppSettings = Settings::CppGenerator;

	if (Type == EFileType::SdkHpp || Type == EFileType::NameCollisionsInl || Type == EFileType::UnrealContainers || Type == EFileType::UnicodeLib)
		return; /* No namespace or packing in SDK.hpp or NameCollisions.inl */

	if constexpr (CppSettings::SDKNamespaceName || CppSettings::ParamNamespaceName)
	{
		if (Type != EFileType::Functions)
			File << "\n";

		File << "}\n\n";
	}
}

void CppGenerator::Generate()
{
	// Generate SDK.hpp with sorted packages
	StreamType SdkHpp(MainFolder / "SDK.hpp");
	GenerateSDKHeader(SdkHpp);

	// Generate PropertyFixup.hpp
	StreamType PropertyFixup(MainFolder / "PropertyFixup.hpp");
	GeneratePropertyFixupFile(PropertyFixup);

	// Generate NameCollisions.inl file containing forward declarations for classes in namespaces (potentially requires lock)
	StreamType NameCollisionsInl(MainFolder / "NameCollisions.inl");
	GenerateNameCollisionsInl(NameCollisionsInl);

	// Generate UnrealContainers.hpp
	StreamType UnrealContainers(MainFolder / "UnrealContainers.hpp");
	GenerateUnrealContainers(UnrealContainers);

	// Generate UtfN.hpp
	StreamType UnicodeLib(MainFolder / "UtfN.hpp");
	GenerateUnicodeLib(UnicodeLib);

	// Generate Basic.hpp and Basic.cpp files
	StreamType BasicHpp(Subfolder / "Basic.hpp");
	StreamType BasicCpp(Subfolder / "Basic.cpp");
	GenerateBasicFiles(BasicHpp, BasicCpp);


	if constexpr (Settings::Debug::bGenerateAssertionFile)
	{
		// Generate Assertions.inl file containing assertions on struct-size, struct-align and member offsets
		StreamType DebugAssertions(MainFolder / "Assertions.inl");
		GenerateDebugAssertions(DebugAssertions);
	}

	// Generates all packages and writes them to files
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		const std::string FileName = Settings::CppGenerator::FilePrefix + Package.GetName();
		const std::u8string U8FileName = reinterpret_cast<const std::u8string&>(FileName);

		StreamType ClassesFile;
		StreamType StructsFile;
		StreamType ParametersFile;
		StreamType FunctionsFile;

		/* Create files and handles namespaces and includes */
		if (Package.HasClasses())
		{
			ClassesFile = StreamType(Subfolder / (U8FileName + u8"_classes.hpp"));

			if (!ClassesFile.is_open())
				std::cout << "Error opening file \"" << (FileName + "_classes.hpp") << "\"" << std::endl;

			WriteFileHead(ClassesFile, Package, EFileType::Classes);

			/* Write enum foward declarations before all of the classes */
			GenerateEnumFwdDeclarations(ClassesFile, Package, true);
		}

		if (Package.HasStructs() || Package.HasEnums())
		{
			StructsFile = StreamType(Subfolder / (U8FileName + u8"_structs.hpp"));

			if (!StructsFile.is_open())
				std::cout << "Error opening file \"" << (FileName + "_structs.hpp") << "\"" << std::endl;

			WriteFileHead(StructsFile, Package, EFileType::Structs);

			/* Write enum foward declarations before all of the structs */
			GenerateEnumFwdDeclarations(StructsFile, Package, false);
		}

		if (Package.HasParameterStructs())
		{
			ParametersFile = StreamType(Subfolder / (U8FileName + u8"_parameters.hpp"));

			if (!ParametersFile.is_open())
				std::cout << "Error opening file \"" << (FileName + "_parameters.hpp") << "\"" << std::endl;

			WriteFileHead(ParametersFile, Package, EFileType::Parameters);
		}

		if (Package.HasFunctions())
		{
			FunctionsFile = StreamType(Subfolder / (U8FileName + u8"_functions.cpp"));

			if (!FunctionsFile.is_open())
				std::cout << "Error opening file \"" << (FileName + "_functions.cpp") << "\"" << std::endl;

			WriteFileHead(FunctionsFile, Package, EFileType::Functions);
		}

		const int32 PackageIndex = Package.GetIndex();

		/* 
		* Generate classes/structs/enums/functions directly into the respective files
		* 
		* Note: Some filestreams aren't opened but passed as parameters anyway because the function demands it, they are not used if they are closed
		*/
		for (int32 EnumIdx : Package.GetEnums())
		{
			GenerateEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx), StructsFile);
		}

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			DependencyManager::OnVisitCallbackType GenerateStructCallback = [&](int32 Index) -> void
			{
				GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructsFile, FunctionsFile, ParametersFile, PackageIndex);
			};

			Structs.VisitAllNodesWithCallback(GenerateStructCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();

			DependencyManager::OnVisitCallbackType GenerateClassCallback = [&](int32 Index) -> void
			{
				GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index), ClassesFile, FunctionsFile, ParametersFile, PackageIndex);
			};

			Classes.VisitAllNodesWithCallback(GenerateClassCallback);
		}


		/* Closes any namespaces if required */
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


	if (Off::InSDK::ULevel::Actors != -1)
	{
		UEClass Level = ObjectArray::FindClassFast("Level");

		if (Level == nullptr)
			Level = ObjectArray::FindClassFast("level");

		PredefinedElements& ULevelPredefs = PredefinedMembers[Level.GetIndex()];
		ULevelPredefs.Members =
		{
			PredefinedMember {
				.Comment = "THIS IS THE ARRAY YOU'RE LOOKING FOR! [NOT AUTO-GENERATED PROPERTY]",
				.Type = "class TArray<class AActor*>", .Name = "Actors", .Offset = Off::InSDK::ULevel::Actors, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};
	}

	UEClass DataTable = ObjectArray::FindClassFast("DataTable");

	PredefinedElements& UDataTablePredefs = PredefinedMembers[DataTable.GetIndex()];
	UDataTablePredefs.Members =
	{
		PredefinedMember {
			.Comment = "So, here's a RowMap. Good luck with it.",
			.Type = "TMap<class FName, uint8*>", .Name = "RowMap", .Offset = Off::InSDK::UDataTable::RowMap, .Size = 0x50, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UObjectPredefs = PredefinedMembers[ObjectArray::FindClassFast("Object").GetIndex()];
	UObjectPredefs.Members = 
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "inline class TUObjectArrayWrapper", .Name = "GObjects", .Offset = 0x0, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "void*", .Name = "VTable", .Offset = Off::UObject::Vft, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "EObjectFlags", .Name = "Flags", .Offset = Off::UObject::Flags, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Index", .Offset = Off::UObject::Index, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "Class", .Offset = Off::UObject::Class, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FName", .Name = "Name", .Offset = Off::UObject::Name, .Size = Off::InSDK::Name::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "Outer", .Offset = Off::UObject::Outer, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UFieldPredefs = PredefinedMembers[ObjectArray::FindClassFast("Field").GetIndex()];
	UFieldPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UField*", .Name = "Next", .Offset = Off::UField::Next, .Size = 0x8, .ArrayDim = 0x1, .Alignment =0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UEnumPredefs = PredefinedMembers[ObjectArray::FindClassFast("Enum").GetIndex()];
	UEnumPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class TArray<class TPair<class FName, int64>>", .Name = "Names", .Offset = Off::UEnum::Names, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	UEObject UStruct = ObjectArray::FindClassFast("Struct");

	if (UStruct == nullptr)
		UStruct = ObjectArray::FindClassFast("struct");

	PredefinedElements& UStructPredefs = PredefinedMembers[UStruct.GetIndex()];
	UStructPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UStruct*", .Name = "Super", .Offset = Off::UStruct::SuperStruct, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UField*", .Name = "Children", .Offset = Off::UStruct::Children, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "MinAlignemnt", .Offset = Off::UStruct::MinAlignemnt, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Size", .Offset = Off::UStruct::Size, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
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
			.Type = "int32", .Name = "ArrayDim", .Offset = Off::Property::ArrayDim, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ElementSize", .Offset = Off::Property::ElementSize, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false,  .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint64", .Name = "PropertyFlags", .Offset = Off::Property::PropertyFlags, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Offset", .Offset = Off::Property::Offset_Internal, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> BytePropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UEnum*", .Name = "Enum", .Offset = Off::ByteProperty::Enum, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> BoolPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "FieldSize", .Offset = Off::BoolProperty::Base, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "ByteOffset", .Offset = Off::BoolProperty::Base + 0x1, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false,  .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "ByteMask", .Offset = Off::BoolProperty::Base + 0x2, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint8", .Name = "FieldMask", .Offset = Off::BoolProperty::Base + 0x3, .Size = 0x01, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ObjectPropertyBaseMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "PropertyClass", .Offset = Off::ObjectProperty::PropertyClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ClassPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "MetaClass", .Offset = Off::ClassProperty::MetaClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> StructPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UStruct*", .Name = "Struct", .Offset = Off::StructProperty::Struct, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> ArrayPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "InnerProperty", .Offset = Off::ArrayProperty::Inner, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> DelegatePropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UFunction*", .Name = "SignatureFunction", .Offset = Off::DelegateProperty::SignatureFunction, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> MapPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "KeyProperty", .Offset = Off::MapProperty::Base, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "ValueProperty", .Offset = Off::MapProperty::Base + 0x8, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> SetPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "ElementProperty", .Offset = Off::SetProperty::ElementProp, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> EnumPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "UnderlayingProperty", .Offset = Off::EnumProperty::Base, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UEnum*", .Name = "Enum", .Offset = Off::EnumProperty::Base + 0x8, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> FieldPathPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FFieldClass*", .Name = "FieldClass", .Offset = Off::FieldPathProperty::FieldClass, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	std::vector<PredefinedMember> OptionalPropertyMembers =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = PropertyTypePtr, .Name = "ValueProperty", .Offset = Off::OptionalProperty::ValueProperty, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	SortMembers(UObjectPredefs.Members);
	SortMembers(UFieldPredefs.Members);
	SortMembers(UEnumPredefs.Members);
	SortMembers(UStructPredefs.Members);
	SortMembers(UFunctionPredefs.Members);
	SortMembers(UClassPredefs.Members);

	SortMembers(PropertyMembers);
	SortMembers(BytePropertyMembers);
	SortMembers(BoolPropertyMembers);
	SortMembers(ObjectPropertyBaseMembers);
	SortMembers(ClassPropertyMembers);
	SortMembers(StructPropertyMembers);
	SortMembers(ArrayPropertyMembers);
	SortMembers(DelegatePropertyMembers);
	SortMembers(MapPropertyMembers);
	SortMembers(SetPropertyMembers);
	SortMembers(FieldPathPropertyMembers);
	SortMembers(OptionalPropertyMembers);

	if (!Settings::Internal::bUseFProperty)
	{
		auto AssignValueIfObjectIsFound = [](const std::string& ClassName, std::vector<PredefinedMember>&& Members) -> void
		{
			if (UEClass Class = ObjectArray::FindClassFast(ClassName))
				PredefinedMembers[Class.GetIndex()].Members = std::move(Members);
		};

		AssignValueIfObjectIsFound("Property", std::move(PropertyMembers));
		AssignValueIfObjectIsFound("ByteProperty", std::move(BytePropertyMembers));
		AssignValueIfObjectIsFound("BoolProperty", std::move(BoolPropertyMembers));
		AssignValueIfObjectIsFound("ObjectPropertyBase", std::move(ObjectPropertyBaseMembers));
		AssignValueIfObjectIsFound("ClassProperty", std::move(ClassPropertyMembers));
		AssignValueIfObjectIsFound("StructProperty", std::move(StructPropertyMembers));
		AssignValueIfObjectIsFound("DelegateProperty", std::move(DelegatePropertyMembers));
		AssignValueIfObjectIsFound("ArrayProperty", std::move(ArrayPropertyMembers));
		AssignValueIfObjectIsFound("MapProperty", std::move(MapPropertyMembers));
		AssignValueIfObjectIsFound("SetProperty", std::move(SetPropertyMembers));
		AssignValueIfObjectIsFound("EnumProperty", std::move(EnumPropertyMembers));

		// FieldPathProperty and OptionalProperty don't exist in UE versions which are using UProperty
		return;
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
			.UniqueName = "FProperty", .Size = Off::InSDK::Properties::PropertySize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .Super = &FField  /* FField */
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
		PredefinedStruct& FDelegateProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FDelegateProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
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
		PredefinedStruct& FFieldPathProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FFieldPathProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});
		PredefinedStruct& FOptionalProperty = PredefinedStructs.emplace_back(PredefinedStruct{
			.UniqueName = "FOptionalProperty", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .Super = &FProperty  /* FProperty */
		});

		FFieldClass.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "FName", .Name = "Name", .Offset = Off::FFieldClass::Name, .Size = Off::InSDK::Name::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
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
		std::string UObjectIdentifierType = (Settings::Internal::bUseMaskForFieldOwner ? "constexpr uint64" : "bool");
		std::string UObjectIdentifierName = (Settings::Internal::bUseMaskForFieldOwner ? "UObjectMask = 0x1" : "bIsUObject");

		FFieldVariant.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "using ContainerType = union { class FField* Field; class UObject* Object; }", .Name = "", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "ContainerType", .Name = "Container", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
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
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
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
				.Type = "FName", .Name = "Name", .Offset = Off::FField::Name, .Size = Off::InSDK::Name::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
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
		FDelegateProperty.Properties = DelegatePropertyMembers;
		FMapProperty.Properties = MapPropertyMembers;
		FSetProperty.Properties = SetPropertyMembers;
		FEnumProperty.Properties = EnumPropertyMembers;
		FFieldPathProperty.Properties = FieldPathPropertyMembers;
		FOptionalProperty.Properties = OptionalPropertyMembers;

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
		InitStructSize(FDelegateProperty);
		InitStructSize(FMapProperty);
		InitStructSize(FSetProperty);
		InitStructSize(FEnumProperty);
		InitStructSize(FFieldPathProperty);
		InitStructSize(FOptionalProperty);
	}
}


void CppGenerator::InitPredefinedFunctions()
{
	static auto SortFunctions = [](std::vector<PredefinedFunction>& Functions) -> void
	{
		std::sort(Functions.begin(), Functions.end(), ComparePredefinedFunctions);
	};

	PredefinedElements& UObjectPredefs = PredefinedMembers[ObjectArray::FindClassFast("Object").GetIndex()];

	UObjectPredefs.Functions =
	{
		/* static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Finds a UObject in the global object array by full-name, optionally with ECastFlags to reduce heavy string comparison",
			.ReturnType = "class UObject*", .NameWithParams = "FindObjectImpl(const std::string& FullName, EClassCastFlags RequiredType = EClassCastFlags::None)",
			.NameWithParamsWithoutDefaults = "FindObjectImpl(const std::string& FullName, EClassCastFlags RequiredType)", .Body =
R"({
	for (int i = 0; i < GObjects->Num(); ++i)
	{
		UObject* Object = GObjects->GetByIndex(i);
	
		if (!Object)
			continue;
		
		if (Object->HasTypeFlag(RequiredType) && Object->GetFullName() == FullName)
			return Object;
	}

	return nullptr;
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "Finds a UObject in the global object array by name, optionally with ECastFlags to reduce heavy string comparison",
			.ReturnType = "class UObject*", .NameWithParams = "FindObjectFastImpl(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None)",
			.NameWithParamsWithoutDefaults = "FindObjectFastImpl(const std::string& Name, EClassCastFlags RequiredType)", .Body =
R"({
	for (int i = 0; i < GObjects->Num(); ++i)
	{
		UObject* Object = GObjects->GetByIndex(i);
	
		if (!Object)
			continue;
		
		if (Object->HasTypeFlag(RequiredType) && Object->GetName() == Name)
			return Object;
	}

	return nullptr;
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = false
		},

		/* static inline functions */
		PredefinedFunction {
			.CustomComment = "",
			.CustomTemplateText = "template<typename UEType = UObject>",
			.ReturnType = "UEType*", .NameWithParams = "FindObject(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None)", .Body =
R"({
	return static_cast<UEType*>(FindObjectImpl(Name, RequiredType));
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.CustomTemplateText = "template<typename UEType = UObject>",
			.ReturnType = "UEType*", .NameWithParams = "FindObjectFast(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None)", .Body =
R"({
	return static_cast<UEType*>(FindObjectFastImpl(Name, RequiredType));
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UClass*", .NameWithParams = "FindClass(const std::string& ClassFullName)", .Body =
R"({
	return FindObject<class UClass>(ClassFullName, EClassCastFlags::Class);
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UClass*", .NameWithParams = "FindClassFast(const std::string& ClassName)", .Body =
R"({
	return FindObjectFast<class UClass>(ClassName, EClassCastFlags::Class);
}
)",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
		},


		/* non-static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Retuns the name of this object",
			.ReturnType = "std::string", .NameWithParams = "GetName()", .Body =
R"({
	return this ? Name.ToString() : "None";
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "Returns the name of this object in the format 'Class Package.Outer.Object'",
			.ReturnType = "std::string", .NameWithParams = "GetFullName()", .Body =
R"({
	if (this && Class)
	{
		std::string Temp;

		for (UObject* NextOuter = Outer; NextOuter; NextOuter = NextOuter->Outer)
		{
			Temp = NextOuter->GetName() + "." + Temp;
		}

		std::string Name = Class->GetName();
		Name += " ";
		Name += Temp;
		Name += GetName();

		return Name;
	}

	return "None";
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction{
			.CustomComment = "Checks a UObjects' type by Class",
			.ReturnType = "bool", .NameWithParams = "IsA(class UClass* TypeClass)", .Body =
R"({
	return Class->IsSubclassOf(TypeClass);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction{
			.CustomComment = "Checks a UObjects' type by TypeFlags",
			.ReturnType = "bool", .NameWithParams = "IsA(EClassCastFlags TypeFlags)", .Body =
R"({
	return (Class->CastFlags & TypeFlags);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction{
			.CustomComment = "Checks Class->FunctionFlags for TypeFlags",
			.ReturnType = "bool", .NameWithParams = "HasTypeFlag(EClassCastFlags TypeFlags)", .Body =
R"({
	return (Class->CastFlags & TypeFlags);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction{
			.CustomComment = "Checks whether this object is a classes' default-object",
			.ReturnType = "bool", .NameWithParams = "IsDefaultObject()", .Body =
		R"({
	return (Flags & EObjectFlags::ClassDefaultObject);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},

		/* non-static inline functions */
		PredefinedFunction{
			.CustomComment = "Unreal Function to process all UFunction-calls",
			.ReturnType = "void", .NameWithParams = "ProcessEvent(class UFunction* Function, void* Parms)", .Body =
R"({
	InSDKUtils::CallGameFunction(InSDKUtils::GetVirtualFunction<void(*)(const UObject*, class UFunction*, void*)>(this, Offsets::ProcessEventIdx), this, Function, Parms);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
	};

	UEClass Struct = ObjectArray::FindClassFast("Struct");

	const int32 UStructIdx = Struct ? Struct.GetIndex() : ObjectArray::FindClassFast("struct").GetIndex(); // misspelled on some UE versions.

	PredefinedElements& UStructPredefs = PredefinedMembers[UStructIdx];

	UStructPredefs.Functions =
	{
		PredefinedFunction {
			.CustomComment = "Checks if this class has a certain base",
			.ReturnType = "bool", .NameWithParams = "IsSubclassOf(const UStruct* Base)", .Body =
R"({
	if (!Base)
		return false;

	for (const UStruct* Struct = this; Struct; Struct = Struct->Super)
	{
		if (Struct == Base)
			return true;
	}

	return false;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
	};

	PredefinedElements& UClassPredefs = PredefinedMembers[ObjectArray::FindClassFast("Class").GetIndex()];

	UClassPredefs.Functions =
	{
		/* non-static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Gets a UFunction from this UClasses' 'Children' list",
			.ReturnType = "class UFunction*", .NameWithParams = "GetFunction(const std::string& ClassName, const std::string& FuncName)", .Body =
R"({
	for(const UStruct* Clss = this; Clss; Clss = Clss->Super)
	{
		if (Clss->GetName() != ClassName)
			continue;
			
		for (UField* Field = Clss->Children; Field; Field = Field->Next)
		{
			if(Field->HasTypeFlag(EClassCastFlags::Function) && Field->GetName() == FuncName)
				return static_cast<class UFunction*>(Field);
		}
	}

	return nullptr;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
	};


	PredefinedElements& UEnginePredefs = PredefinedMembers[ObjectArray::FindClassFast("Engine").GetIndex()];

	UEnginePredefs.Functions =
	{
		/* static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Gets a pointer to a valid UObject of type UEngine",
			.ReturnType = "class UEngine*", .NameWithParams = "GetEngine()", .Body =
R"({
	static UEngine* GEngine = nullptr;

	if (GEngine)
		return GEngine;
	
	/* (Re-)Initialize if GEngine is nullptr */
	for (int i = 0; i < UObject::GObjects->Num(); i++)
	{
		UObject* Obj = UObject::GObjects->GetByIndex(i);

		if (!Obj)
			continue;

		if (Obj->IsA(UEngine::StaticClass()) && !Obj->IsDefaultObject())
		{
			GEngine = static_cast<UEngine*>(Obj);
			break;
		}
	}

	return GEngine; 
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = false
		},
	};


	PredefinedElements& UGameEnginePredefs = PredefinedMembers[ObjectArray::FindClassFast("GameEngine").GetIndex()];

	UGameEnginePredefs.Functions =
	{
		/* static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Returns the result of UEngine::GetEngine() without a type-check, might be dangerous",
			.ReturnType = "class UGameEngine*", .NameWithParams = "GetEngine()", .Body =
R"({
	return static_cast<UGameEngine*>(UEngine::GetEngine());
})",
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = false
		},
	};


	PredefinedElements& UWorldPredefs = PredefinedMembers[ObjectArray::FindClassFast("World").GetIndex()];

	constexpr const char* GetWorldThroughGWorldCode = R"(
	if constexpr (Offsets::GWorld != 0)
		return *reinterpret_cast<UWorld**>(InSDKUtils::GetImageBase() + Offsets::GWorld);
)";

	UWorldPredefs.Functions =
	{
		/* static non-inline functions */
		PredefinedFunction {
			.CustomComment = "Gets a pointer to the current World of the GameViewport",
			.ReturnType = "class UWorld*", .NameWithParams = "GetWorld()", .Body =
std::format(R"({{{}
	if (UEngine* Engine = UEngine::GetEngine())
	{{
		if (!Engine->GameViewport)
			return nullptr;

		return Engine->GameViewport->World;
	}}

	return nullptr;
}})", !Settings::CppGenerator::bForceNoGWorldInSDK ?  GetWorldThroughGWorldCode : ""),
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = false
		},
	};

	UEStruct Vector = ObjectArray::FindObjectFast<UEStruct>("Vector");

	PredefinedElements& FVectorPredefs = PredefinedMembers[Vector.GetIndex()];

	FVectorPredefs.Members.push_back(PredefinedMember{
		PredefinedMember{
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = std::format("using UnderlayingType = {}", (Settings::Internal::bUseLargeWorldCoordinates ? "double" : "float")), .Name = "", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF,
		}
	});

	FVectorPredefs.Functions =
	{
		/* const operators */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator+(const FVector& Other)", .Body =
R"({
	return { X + Other.X, Y + Other.Y, Z + Other.Z };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator-(const FVector& Other)", .Body =
R"({
	return { X - Other.X, Y - Other.Y, Z - Other.Z };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator*(float Scalar)", .Body =
R"({
	return { X * Scalar, Y * Scalar, Z * Scalar };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator*(const FVector& Other)", .Body =
R"({
	return { X * Other.X, Y * Other.Y, Z * Other.Z };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator/(float Scalar)", .Body =
R"({
	if (Scalar == 0.0f)
		return *this;

	return { X / Scalar, Y / Scalar, Z / Scalar };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "operator/(const FVector& Other)", .Body =
R"({
	if (Other.X == 0.0f || Other.Y == 0.0f ||Other.Z == 0.0f)
		return *this;

	return { X / Other.X, Y / Other.Y, Z / Other.Z };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator==(const FVector& Other)", .Body =
R"({
	return X == Other.X && Y == Other.Y && Z == Other.Z;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator!=(const FVector& Other)", .Body =
R"({
	return X != Other.X || Y != Other.Y || Z != Other.Z;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},

		/* Non-const operators */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator+=(const FVector& Other)", .Body =
R"({
	*this = *this + Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator-=(const FVector& Other)", .Body =
R"({
	*this = *this - Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator*=(float Scalar)", .Body =
R"({
	*this = *this * Scalar;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator*=(const FVector& Other)", .Body =
R"({
	*this = *this * Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator/=(float Scalar)", .Body =
R"({
	*this = *this / Scalar;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "operator/=(const FVector& Other)", .Body =
R"({
	*this = *this / Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},

		/* Const functions */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "IsZero()", .Body =
R"({
	return X == 0.0 && Y == 0.0 && Z == 0.0;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "Dot(const FVector& Other)", .Body =
R"({
	return (X * Other.X) + (Y * Other.Y) + (Z * Other.Z);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "Magnitude()", .Body =
R"({
	return std::sqrt((X * X) + (Y * Y) + (Z * Z));
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "FVector", .NameWithParams = "GetNormalized()", .Body =
R"({
	return *this / Magnitude();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "GetDistanceTo(const FVector& Other)", .Body =
R"({
	FVector DiffVector = Other - *this;
	return DiffVector.Magnitude();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "GetDistanceToInMeters(const FVector& Other)", .Body =
R"({
	return GetDistanceTo(Other) * static_cast<UnderlayingType>(0.01);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},


		/* Non-const functions */
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "FVector&", .NameWithParams = "Normalize()", .Body =
R"({
	*this /= Magnitude();
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
	};

	UEStruct Vector2D = ObjectArray::FindObjectFast<UEStruct>("Vector2D");

	PredefinedElements& FVector2DPredefs = PredefinedMembers[Vector2D.GetIndex()];
	FVector2DPredefs.Members.push_back(PredefinedMember{
		PredefinedMember{
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = std::format("using UnderlayingType = {}", (Settings::Internal::bUseLargeWorldCoordinates ? "double" : "float")), .Name = "", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF,
		}
	});

	FVector2DPredefs.Functions =
	{
		/* const operators */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator+(const FVector2D& Other)", .Body =
R"({
	return { X + Other.X, Y + Other.Y };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator-(const FVector2D& Other)", .Body =
R"({
	return { X - Other.X, Y - Other.Y  };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator*(float Scalar)", .Body =
R"({
	return { X * Scalar, Y * Scalar };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator*(const FVector2D& Other)", .Body =
R"({
	return { X * Other.X, Y * Other.Y };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator/(float Scalar)", .Body =
R"({
	if (Scalar == 0.0f)
		return *this;

	return { X / Scalar, Y / Scalar };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "operator/(const FVector2D& Other)", .Body =
R"({
	if (Other.X == 0.0f || Other.Y == 0.0f)
		return *this;

	return { X / Other.X, Y / Other.Y };
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator==(const FVector2D& Other)", .Body =
R"({
	return X == Other.X && Y == Other.Y;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator!=(const FVector2D& Other)", .Body =
R"({
	return X != Other.X || Y != Other.Y;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},

		/* Non-const operators */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator+=(const FVector2D& Other)", .Body =
R"({
	*this = *this + Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator-=(const FVector2D& Other)", .Body =
R"({
	*this = *this - Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator*=(float Scalar)", .Body =
R"({
	*this = *this * Scalar;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator*=(const FVector2D& Other)", .Body =
R"({
	*this = *this * Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator/=(float Scalar)", .Body =
R"({
	*this = *this / Scalar;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "operator/=(const FVector2D& Other)", .Body =
R"({
	*this = *this / Other;
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},

		/* Const functions */
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "IsZero()", .Body =
R"({
	return X == 0.0 && Y == 0.0;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "Dot(const FVector2D& Other)", .Body =
R"({
	return (X * Other.X) + (Y * Other.Y);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "Magnitude()", .Body =
R"({
	return std::sqrt((X * X) + (Y * Y));
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "FVector2D", .NameWithParams = "GetNormalized()", .Body =
R"({
	return *this / Magnitude();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "UnderlayingType", .NameWithParams = "GetDistanceTo(const FVector2D& Other)", .Body =
R"({
	FVector2D DiffVector = Other - *this;
	return DiffVector.Magnitude();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},


		/* Non-const functions */
		PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "FVector2D&", .NameWithParams = "Normalize()", .Body =
R"({
	*this /= Magnitude();
	return *this;
})",
			.bIsStatic = false, .bIsConst = false, .bIsBodyInline = true
		},
	};

	SortFunctions(UObjectPredefs.Functions);
	SortFunctions(UClassPredefs.Functions);
	SortFunctions(UEnginePredefs.Functions);
	SortFunctions(UGameEnginePredefs.Functions);
	SortFunctions(UWorldPredefs.Functions);
	SortFunctions(FVectorPredefs.Functions);
	SortFunctions(FVector2DPredefs.Functions);
}


void CppGenerator::GenerateBasicFiles(StreamType& BasicHpp, StreamType& BasicCpp)
{
	namespace CppSettings = Settings::CppGenerator;

	static auto SortMembers = [](std::vector<PredefinedMember>& Members) -> void
	{
		std::sort(Members.begin(), Members.end(), ComparePredefinedMembers);
	};

	std::string CustomIncludes = R"(#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <functional>
#include <type_traits>
)";

	WriteFileHead(BasicHpp, nullptr, EFileType::BasicHpp, "Basic file containing structs required by the SDK", CustomIncludes);
	WriteFileHead(BasicCpp, nullptr, EFileType::BasicCpp, "Basic file containing function-implementations from Basic.hpp", "#include <Windows.h>");


	/* use namespace of UnrealContainers */
	BasicHpp <<
		R"(
using namespace UC;
)";

	BasicHpp << "\n#include \"../NameCollisions.inl\"\n";

	/* Offsets and disclaimer */
	BasicHpp << std::format(R"(
/*
* Disclaimer:
*	- The 'GNames' is only a fallback and null by default, FName::AppendString is used
*	- THe 'GWorld' offset is not used by the SDK, it's just there for "decoration", use the provided 'UWorld::GetWorld()' function instead
*/
namespace Offsets
{{
	constexpr int32 GObjects          = 0x{:08X};
	constexpr int32 AppendString      = 0x{:08X};
	constexpr int32 GNames            = 0x{:08X};
	constexpr int32 GWorld            = 0x{:08X};
	constexpr int32 ProcessEvent      = 0x{:08X};
	constexpr int32 ProcessEventIdx   = 0x{:08X};
}}
)", Off::InSDK::ObjArray::GObjects, Off::InSDK::Name::AppendNameToString, Off::InSDK::NameArray::GNames, Off::InSDK::World::GWorld, Off::InSDK::ProcessEvent::PEOffset, Off::InSDK::ProcessEvent::PEIndex);



	// Start Namespace 'InSDKUtils'
	BasicHpp <<
		R"(
namespace InSDKUtils
{
)";


	/* Custom 'GetImageBase' function */
	BasicHpp << "\tuintptr_t GetImageBase();\n\n";


	/* GetVirtualFunction(const void* ObjectInstance, int32 Index) function */
	BasicHpp << R"(	template<typename FuncType>
	inline FuncType GetVirtualFunction(const void* ObjectInstance, int32 Index)
	{
		void** VTable = *reinterpret_cast<void***>(const_cast<void*>(ObjectInstance));

		return reinterpret_cast<FuncType>(VTable[Index]);
	}
)";

	//Customizable part of Cpp code to allow for a custom 'CallGameFunction' function
	BasicHpp << CppSettings::CallGameFunction;

	BasicHpp << "}\n\n";
	// End Namespace 'InSDKUtils'

	/* Custom 'GetImageBase' function */
	BasicCpp << std::format(R"(uintptr_t InSDKUtils::GetImageBase()
{})", Settings::CppGenerator::GetImageBaseFuncBody);

	if constexpr (!Settings::CppGenerator::XORString)
	{
		/* Utility class for contexpr string literals. Used for 'StaticClassImpl<"ClassName">()'. */
		BasicHpp << R"(
template<int32 Len>
struct StringLiteral
{
	char Chars[Len];

	consteval StringLiteral(const char(&String)[Len])
	{
		std::copy_n(String, Len, Chars);
	}

	operator std::string() const
	{
		return static_cast<const char*>(Chars);
	}
};
)";
	}
	else
	{
		/* Utility class for constexpr strings encrypted with https://github.com/JustasMasiulis/xorstr. Other xorstr implementations may need custom-implementations. */
		BasicHpp << R"(
template<typename XorStrType>
struct StringLiteral
{
	XorStrType EncryptedString;

	consteval StringLiteral(XorStrType Str)
		: EncryptedString(Str)
	{
	}

	operator std::string() const
	{
		return EncryptedString.crypt_get();
	}
};
)";
	}

	BasicHpp << R"(
// Forward declarations because in-line forward declarations make the compiler think 'StaticClassImpl()' is a class template
class UClass;
class UObject;
class UFunction;

class FName;
)";

	BasicHpp << R"(
namespace BasicFilesImpleUtils
{
	// Helper functions for StaticClassImpl and StaticBPGeneratedClassImpl
	UClass* FindClassByName(const std::string& Name);
	UClass* FindClassByFullName(const std::string& Name);

	std::string GetObjectName(class UClass* Class);
	int32 GetObjectIndex(class UClass* Class);

	/* FName represented as a uint64. */
	uint64 GetObjFNameAsUInt64(class UClass* Class);

	UObject* GetObjectByIndex(int32 Index);

	UFunction* FindFunctionByFName(const FName* Name);
}
)";

	BasicCpp << R"(
class UClass* BasicFilesImpleUtils::FindClassByName(const std::string& Name)
{
	return UObject::FindClassFast(Name);
}

class UClass* BasicFilesImpleUtils::FindClassByFullName(const std::string& Name)
{
	return UObject::FindClass(Name);
}

std::string BasicFilesImpleUtils::GetObjectName(class UClass* Class)
{
	return Class->GetName();
}

int32 BasicFilesImpleUtils::GetObjectIndex(class UClass* Class)
{
	return Class->Index;
}

uint64 BasicFilesImpleUtils::GetObjFNameAsUInt64(class UClass* Class)
{
	return *reinterpret_cast<uint64*>(&Class->Name);
}

class UObject* BasicFilesImpleUtils::GetObjectByIndex(int32 Index)
{
	return UObject::GObjects->GetByIndex(Index);
}

UFunction* BasicFilesImpleUtils::FindFunctionByFName(const FName* Name)
{
	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->Name == *Name)
			return static_cast<UFunction*>(Object);
	}

	return nullptr;
}

)";

	/* Implementation of 'UObject::StaticClass()', templated to allow for a per-class local static class-pointer */
	BasicHpp << R"(
template<StringLiteral Name, bool bIsFullName = false>
class UClass* StaticClassImpl()
{
	static class UClass* Clss = nullptr;

	if (Clss == nullptr)
	{
		if constexpr (bIsFullName) {
			Clss = BasicFilesImpleUtils::FindClassByFullName(Name);
		}
		else /* default */ {
			Clss = BasicFilesImpleUtils::FindClassByName(Name);
		}
	}

	return Clss;
}
)";

	/* Implementation of 'UObject::StaticClass()' for 'BlueprintGeneratedClass', templated to allow for a per-class local static class-index */
	BasicHpp << R"(
template<StringLiteral Name, bool bIsFullName = false, StringLiteral NonFullName = "">
class UClass* StaticBPGeneratedClassImpl()
{
	/* Could be external function, not really unique to this StaticClass functon */
	static auto SetClassIndex = [](UClass* Class, int32& Index, uint64& ClassName) -> UClass*
	{
		if (Class)
		{
			Index = BasicFilesImpleUtils::GetObjectIndex(Class);
			ClassName = BasicFilesImpleUtils::GetObjFNameAsUInt64(Class);
		}

		return Class;
	};

	static int32 ClassIdx = 0x0;
	static uint64 ClassName = 0x0;

	/* Use the full name to find an object */
	if constexpr (bIsFullName)
	{
		if (ClassIdx == 0x0) [[unlikely]]
			return SetClassIndex(BasicFilesImpleUtils::FindClassByFullName(Name), ClassIdx, ClassName);

		UClass* ClassObj = static_cast<UClass*>(BasicFilesImpleUtils::GetObjectByIndex(ClassIdx));

		/* Could use cast flags too to save some string comparisons */
		if (!ClassObj || BasicFilesImpleUtils::GetObjFNameAsUInt64(ClassObj) != ClassName)
			return SetClassIndex(BasicFilesImpleUtils::FindClassByFullName(Name), ClassIdx, ClassName);

		return ClassObj;
	}
	else /* Default, use just the name to find an object*/
	{
		if (ClassIdx == 0x0) [[unlikely]]
			return SetClassIndex(BasicFilesImpleUtils::FindClassByName(Name), ClassIdx, ClassName);

		UClass* ClassObj = static_cast<UClass*>(BasicFilesImpleUtils::GetObjectByIndex(ClassIdx));

		/* Could use cast flags too to save some string comparisons */
		if (!ClassObj || BasicFilesImpleUtils::GetObjFNameAsUInt64(ClassObj) != ClassName)
			return SetClassIndex(BasicFilesImpleUtils::FindClassByName(Name), ClassIdx, ClassName);

		return ClassObj;
	}
}
)";

	/* Implementation of 'UObject::StaticClass()', templated to allow for a per-object local static */
	BasicHpp << R"(
template<class ClassType>
ClassType* GetDefaultObjImpl()
{
	return static_cast<ClassType*>(ClassType::StaticClass()->DefaultObject);
}

)";

	// Start class 'FUObjectItem'
	PredefinedStruct FUObjectItem = PredefinedStruct{
		.UniqueName = "FUObjectItem", .Size = Off::InSDK::ObjArray::FUObjectItemSize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
	};

	FUObjectItem.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "Object", .Offset = Off::InSDK::ObjArray::FUObjectItemInitialOffset, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	GenerateStruct(&FUObjectItem, BasicHpp, BasicCpp, BasicHpp);

	constexpr const char* DefaultDecryption = R"([](void* ObjPtr) -> uint8*
	{
		return reinterpret_cast<uint8*>(ObjPtr);
	})";

	std::string DecryptionStrToUse = ObjectArray::DecryptionLambdaStr.empty() ? DefaultDecryption : std::move(ObjectArray::DecryptionLambdaStr);
//#error Fix this and fix alignof(UObject) == 0x1
	if (Off::InSDK::ObjArray::ChunkSize <= 0)
	{
#undef max
		const auto& ObjectsArrayLayout = Off::FUObjectArray::FixedLayout;
		const int32 ObjectArraySize = std::max({ ObjectsArrayLayout.ObjectsOffset + 0x8, ObjectsArrayLayout.NumObjectsOffset + 0x4, ObjectsArrayLayout.MaxObjectsOffset + 0x4, });
#define max(A, B) (A > B ? A : B)

		// Start class 'TUObjectArray'
		PredefinedStruct TUObjectArray = PredefinedStruct{
			.UniqueName = "TUObjectArray", .Size = ObjectArraySize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
		};

		TUObjectArray.Properties =
		{
			/* Static members of TUObjectArray */
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = ("static constexpr auto DecryptPtr = " + DecryptionStrToUse), .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},

			/* Non-static members of TUObjectArray */
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "struct FUObjectItem*", .Name = "Objects", .Offset = ObjectsArrayLayout.ObjectsOffset, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "MaxElements", .Offset = ObjectsArrayLayout.MaxObjectsOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NumElements", .Offset = ObjectsArrayLayout.NumObjectsOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		TUObjectArray.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "inline int32", .NameWithParams = "Num()", .Body =
R"({
	return NumElements;
}
)",				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "FUObjectItem*", .NameWithParams = "GetDecrytedObjPtr()", .Body =
R"({
	return reinterpret_cast<FUObjectItem*>(DecryptPtr(Objects));
}
)",				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "inline class UObject*", .NameWithParams = "GetByIndex(const int32 Index)", .Body =
R"({
	if (Index < 0 || Index > NumElements)
		return nullptr;

	return GetDecrytedObjPtr()[Index].Object;
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		SortMembers(TUObjectArray.Properties);
		GenerateStruct(&TUObjectArray, BasicHpp, BasicCpp, BasicHpp);
	}
	else
	{

#undef max
		const auto& ObjectsArrayLayout = Off::FUObjectArray::ChunkedFixedLayout;
		const int32 ObjectArraySize = std::max({
			ObjectsArrayLayout.ObjectsOffset + 0x8,
			ObjectsArrayLayout.MaxElementsOffset + 0x8,
			ObjectsArrayLayout.NumElementsOffset + 0x4,
			ObjectsArrayLayout.MaxChunksOffset + 0x4,
			ObjectsArrayLayout.NumChunksOffset + 0x4
		});
#define max(A, B) (A > B ? A : B)

		// Start class 'TUObjectArray'
		PredefinedStruct TUObjectArray = PredefinedStruct{
			.UniqueName = "TUObjectArray", .Size = ObjectArraySize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
		};

		TUObjectArray.Properties =
		{
			/* Static members of TUObjectArray */
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = ("static constexpr auto DecryptPtr = " + DecryptionStrToUse), .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr int32", .Name = "ElementsPerChunk", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:X}", Off::InSDK::ObjArray::ChunkSize)
			},

			/* Non-static members of TUObjectArray */
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "struct FUObjectItem**", .Name = "Objects", .Offset = ObjectsArrayLayout.ObjectsOffset, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "MaxElements", .Offset = ObjectsArrayLayout.MaxElementsOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NumElements", .Offset = ObjectsArrayLayout.NumElementsOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "MaxChunks", .Offset = ObjectsArrayLayout.MaxChunksOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NumChunks", .Offset = ObjectsArrayLayout.NumChunksOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		TUObjectArray.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "inline int32", .NameWithParams = "Num()", .Body =
R"({
	return NumElements;
}
)",				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "FUObjectItem**", .NameWithParams = "GetDecrytedObjPtr()", .Body =
R"({
	return reinterpret_cast<FUObjectItem**>(DecryptPtr(Objects));
}
)",				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "inline class UObject*", .NameWithParams = "GetByIndex(const int32 Index)", .Body =
R"({
	const int32 ChunkIndex = Index / ElementsPerChunk;
	const int32 InChunkIdx = Index % ElementsPerChunk;
	
	if (Index < 0 || ChunkIndex >= NumChunks || Index >= NumElements)
	    return nullptr;
	
	FUObjectItem* ChunkPtr = GetDecrytedObjPtr()[ChunkIndex];
	if (!ChunkPtr) return nullptr;
	
	return ChunkPtr[InChunkIdx].Object;
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		SortMembers(TUObjectArray.Properties);
		GenerateStruct(&TUObjectArray, BasicHpp, BasicCpp, BasicHpp);
	}
	// End class 'TUObjectArray'



	/* TUObjectArrayWrapper so InitGObjects() doesn't need to be called manually anymore */
	// Start class 'TUObjectArrayWrapper'
	BasicHpp << R"(
class TUObjectArrayWrapper
{
private:
	friend class UObject;

private:
	void* GObjectsAddress = nullptr;

private:
	TUObjectArrayWrapper() = default;

public:
	TUObjectArrayWrapper(TUObjectArrayWrapper&&) = delete;
	TUObjectArrayWrapper(const TUObjectArrayWrapper&) = delete;

	TUObjectArrayWrapper& operator=(TUObjectArrayWrapper&&) = delete;
	TUObjectArrayWrapper& operator=(const TUObjectArrayWrapper&) = delete;

private:
	inline void InitGObjects()
	{
		GObjectsAddress = reinterpret_cast<void*>(InSDKUtils::GetImageBase() + Offsets::GObjects);
	}

public:)";
	if constexpr (Settings::CppGenerator::bAddManualOverrideOptions)
	{
	BasicHpp << R"(
	inline void InitManually(void* GObjectsAddressParameter)
	{
		GObjectsAddress = GObjectsAddressParameter;
	}
)"; }
	BasicHpp << R"(
	inline class TUObjectArray* operator->()
	{
		if (!GObjectsAddress) [[unlikely]]
			InitGObjects();

		return reinterpret_cast<class TUObjectArray*>(GObjectsAddress);
	}

	inline TUObjectArray& operator*() const
	{
		return *reinterpret_cast<class TUObjectArray*>(GObjectsAddress);
	}

	inline operator const void* ()
	{
		if (!GObjectsAddress) [[unlikely]]
			InitGObjects();

		return GObjectsAddress;
	}

	inline class TUObjectArray* GetTypedPtr()
	{
		if (!GObjectsAddress) [[unlikely]]
			InitGObjects();

		return reinterpret_cast<class TUObjectArray*>(GObjectsAddress);
	}
};
)";
	// End class 'TUObjectArrayWrapper'



	/* struct FStringData */
	PredefinedStruct FStringData = PredefinedStruct{
		.UniqueName = "FStringData", .Size = 0x800, .Alignment = 0x2, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = true, .Super = nullptr
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
			.Type = "wchar_t", .Name = "WideName", .Offset = 0x0, .Size = 0x02, .ArrayDim = 0x400, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	
	if (Off::InSDK::Name::AppendNameToString == 0x0 && !Settings::Internal::bUseNamePool)
	{
		/* struct FNameEntry */
		PredefinedStruct FNameEntry = PredefinedStruct{
			.UniqueName = "FNameEntry", .Size = Off::FNameEntry::NameArray::StringOffset + 0x800, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		FNameEntry.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr int32", .Name = "NameWideMask", .Offset = 0x0, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "0x1"
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NameIndex", .Offset = Off::FNameEntry::NameArray::IndexOffset, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "union FStringData", .Name = "Name", .Offset = Off::FNameEntry::NameArray::StringOffset, .Size = 0x800, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		SortMembers(FNameEntry.Properties);

		FNameEntry.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "bool", .NameWithParams = "IsWide()", .Body =
R"({
	return (NameIndex & NameWideMask);
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "std::string", .NameWithParams = "GetString()", .Body =
R"({
	if (IsWide())
	{
		return UtfN::Utf16StringToUtf8String<std::string>(Name.WideName, wcslen(Name.WideName));
	}

	return Name.AnsiName;
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		const int32 ChunkTableSize = Off::NameArray::NumElements / 0x8;
		const int32 ChunkTableSizeBytes = ChunkTableSize * 0x8;

		PredefinedStruct TNameEntryArray = PredefinedStruct{
			.UniqueName = "TNameEntryArray", .Size = (ChunkTableSizeBytes + 0x08), .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
		};

		/* class TNameEntryArray */
		TNameEntryArray.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "ChunkTableSize", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:04X}", ChunkTableSize)
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "NumElementsPerChunk", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "0x4000"
			},

			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "FNameEntry**", .Name = "Chunks", .Offset = 0x0, .Size = ChunkTableSizeBytes, .ArrayDim = ChunkTableSize, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NumElements", .Offset = (ChunkTableSizeBytes + 0x0), .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "int32", .Name = "NumChunks", .Offset = (ChunkTableSizeBytes + 0x4), .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		TNameEntryArray.Functions =
		{
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "bool", .NameWithParams = "IsValidIndex(int32 Index, int32 ChunkIdx, int32 InChunkIdx)", .Body =
R"({
	return Index >= 0 && Index < NumElements;
}
)",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
			PredefinedFunction {
				.CustomComment = "",
				.ReturnType = "FNameEntry*", .NameWithParams = "GetEntryByIndex(int32 Index)", .Body =
R"({
	const int32 ChunkIdx = Index / NumElementsPerChunk;
	const int32 InChunk  = Index % NumElementsPerChunk;

	if (!IsValidIndex(Index, ChunkIdx, InChunk))
		return nullptr;

	return Chunks[ChunkIdx][InChunk];
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		GenerateStruct(&FStringData, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&FNameEntry, BasicHpp, BasicCpp, BasicHpp);
		GenerateStruct(&TNameEntryArray, BasicHpp, BasicCpp, BasicHpp);
	}
	else if (Off::InSDK::Name::AppendNameToString == 0x0 && Settings::Internal::bUseNamePool)
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


		if (Settings::Internal::bUseOutlineNumberName)
		{
			FStringData.Properties.push_back(
				PredefinedMember{
					.Comment = "NOT AUTO-GENERATED PROPERTY",
					.Type = "FNumberedData", .Name = "AnsiName", .Offset = 0x10, .Size = FNumberedDataSize, .ArrayDim = 0x400, .Alignment = 0x1,
					.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
				}
			);
		}


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


		/* struct FNameEntry */
		PredefinedStruct FNameEntry = PredefinedStruct{
			.UniqueName = "FNameEntry", .Size = Off::FNameEntry::NamePool::StringOffset + 0x800, .Alignment = 0x2, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		FNameEntry.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "struct FNameEntryHeader", .Name = "Header", .Offset = Off::FNameEntry::NamePool::HeaderOffset, .Size = FNameEntryHeaderSize, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "union FStringData", .Name = "Name", .Offset = Off::FNameEntry::NamePool::HeaderOffset + FNameEntryHeaderSize, .Size = 0x800, .ArrayDim = 0x1, .Alignment = 0x2,
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
		return UtfN::Utf16StringToUtf8String<std::string>(Name.WideName, Header.Len);
	}

	return std::string(Name.AnsiName, Header.Len);
})",
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
			},
		};

		constexpr int32 SizeOfChunkPtrs = 0x2000 * 0x8;

		/* class FNamePool */
		PredefinedStruct FNamePool = PredefinedStruct{
			.UniqueName = "FNamePool", .Size = Off::NameArray::ChunksStart + SizeOfChunkPtrs, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
		};

		FNamePool.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameEntryStride", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:04X}", Off::InSDK::NameArray::FNameEntryStride)
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameBlockOffsetBits", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = std::format("0x{:04X}", Off::InSDK::NameArray::FNamePoolBlockOffsetBits)
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "constexpr uint32", .Name = "FNameBlockOffsets", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "1 << FNameBlockOffsetBits"
			},

			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint32", .Name = "CurrentBlock", .Offset = Off::NameArray::MaxChunkIndex, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x2,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint32", .Name = "CurrentByteCursor", .Offset = Off::NameArray::ByteCursor, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "uint8*", .Name = "Blocks", .Offset = Off::NameArray::ChunksStart, .Size = SizeOfChunkPtrs, .ArrayDim = 0x2000, .Alignment = 0x8,
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


	/* class FName */
	PredefinedStruct FName = PredefinedStruct{
		.UniqueName = "FName", .Size = Off::InSDK::Name::FNameSize, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	std::string NameArrayType = Off::InSDK::Name::AppendNameToString == 0 ? Settings::Internal::bUseNamePool ? "FNamePool*" : "TNameEntryArray*" : "void*";
	std::string NameArrayName = Off::InSDK::Name::AppendNameToString == 0 ? "GNames" : "AppendString";

	FName.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "inline " + NameArrayType, .Name = NameArrayName, .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = true, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "nullptr"
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ComparisonIndex", .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
		},
	};

	/* Add an error message to FName if ToString is used */
	if (!Off::InSDK::Name::bIsUsingAppendStringOverToString)
	{
		FName.Properties.insert(FName.Properties.begin(), {
			PredefinedMember {
				.Comment = "",
				.Type = "/*  Unlike FMemory::AppendString, FName::ToString allocates memory every time it is used.  */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "",
				.Type = "/*  This allocation needs to be freed using UE's FMemory::Free function.                   */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "",
				.Type = "/*  Inside of 'FName::GetRawString':                                                       */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "",
				.Type = "/*  1. Change \"thread_local FAllocatedString TempString(1024);\" to \"FString TempString;\"   */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "",
				.Type = "/*  2. Add a \"Free\" function to FString that calls FMemory::Free(Data);                    */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "",
				.Type = "/*  3. Replace \"TempString.Clear();\" with \"TempString.Free();\"                             */", .Name = NameArrayName, .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF
			},
			PredefinedMember {
				.Comment = "Free your allocation with FMemory::Free!",
				.Type = "static_assert(false, \"FName::ToString causes memory-leak. Read comment above!\")", .Name = NameArrayName, .Offset = 0x0, .Size = 0x4, .ArrayDim = 0x0, .Alignment = 0x4,
				.bIsStatic = true, .bIsZeroSizeMember = true, .bIsBitField = false, .BitIndex = 0xFF, .DefaultValue = "nullptr"
			},
		});
	}

	if (!Settings::Internal::bUseOutlineNumberName)
	{
		FName.Properties.push_back(PredefinedMember{
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = (Settings::Internal::bUseNamePool ? "uint32" : "int32"), .Name = "Number", .Offset = Off::FName::Number, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
			}
		);
	}

	if (Settings::Internal::bUseCasePreservingName)
	{
		const int32 DisplayIndexOffset = Off::FName::Number == 4 ? 0x8 : 0x4;

		FName.Properties.push_back(PredefinedMember{
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "DisplayIndex", .Offset = DisplayIndexOffset, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF,
			}
		);
	}

	SortMembers(FName.Properties);

	constexpr const char* GetRawStringWithAppendString =
		R"({
	thread_local FAllocatedString TempString(1024);

	if (!AppendString)
		InitInternal();

	InSDKUtils::CallGameFunction(reinterpret_cast<void(*)(const FName*, FString&)>(AppendString), this, TempString);

	std::string OutputString = TempString.ToString();
	TempString.Clear();

	return OutputString;
}
)";

	constexpr const char* GetRawStringWithNameArray =
		R"({
	if (!GNames)
		InitInternal();

	std::string RetStr = FName::GNames->GetEntryByIndex(GetDisplayIndex())->GetString();

	if (Number > 0)
		RetStr += ("_" + std::to_string(Number - 1));

	return RetStr;
}
)";

	constexpr const char* GetRawStringWithNameArrayWithOutlineNumber =
		R"({
	if (!GNames)
		InitInternal();

	const FNameEntry* Entry = FName::GNames->GetEntryByIndex(GetDisplayIndex());

	if (Entry->Header.Length == 0)
	{{
		if (Entry->Number > 0)
			return FName::GNames->GetEntryByIndex(Entry->NumberedName.Id)->GetString() + "_" + std::to_string(Entry->Number - 1);

		return FName::GNames->GetEntryByIndex(Entry->NumberedName.Id)->GetString();
	}}

	return Entry.GetString();
}
)";

	std::string GetRawStringBody = Off::InSDK::Name::AppendNameToString == 0 ? Settings::Internal::bUseOutlineNumberName ? GetRawStringWithNameArrayWithOutlineNumber : GetRawStringWithNameArray : GetRawStringWithAppendString;

	FName.Functions =
	{
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "void", .NameWithParams = "InitInternal()", .Body =
std::format(R"({{
	{0} = {2}reinterpret_cast<{1}{2}>(InSDKUtils::GetImageBase() + Offsets::{0});
}})", NameArrayName, NameArrayType, (Off::InSDK::Name::AppendNameToString == 0 && !Settings::Internal::bUseNamePool ? "*" : "")),
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "int32", .NameWithParams = "GetDisplayIndex()", .Body =
std::format(R"({{
	return {};
}}
)", Settings::Internal::bUseCasePreservingName ? "DisplayIndex" : "ComparisonIndex"),
				.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "std::string", .NameWithParams = "GetRawString()", .Body = GetRawStringBody,
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "std::string", .NameWithParams = "ToString()", .Body = R"({
	std::string OutputString = GetRawString();

	size_t pos = OutputString.rfind('/');

	if (pos == std::string::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}
)",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator==(const FName& Other)", .Body =
std::format(R"({{
	return ComparisonIndex == Other.ComparisonIndex{};
}})", !Settings::Internal::bUseOutlineNumberName ? " && Number == Other.Number" : ""),
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator!=(const FName& Other)", .Body =
std::format(R"({{
	return ComparisonIndex != Other.ComparisonIndex{};
}})", !Settings::Internal::bUseOutlineNumberName ? " || Number != Other.Number" : ""),
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
	};

	if constexpr (Settings::CppGenerator::bAddManualOverrideOptions)
	{
		FName.Functions.insert(
			FName.Functions.begin() + 1,
			PredefinedFunction{
			.CustomComment = "",
			.ReturnType = "void", .NameWithParams = "InitManually(void* Location)", .Body =
std::format(R"({{
	{0} = {2}reinterpret_cast<{1}{2}>(Location);
}})", NameArrayName, NameArrayType, (Off::InSDK::Name::AppendNameToString == 0 && !Settings::Internal::bUseNamePool ? "*" : "")),
			.bIsStatic = true, .bIsConst = false, .bIsBodyInline = true
			});
	}

	GenerateStruct(&FName, BasicHpp, BasicCpp, BasicHpp);


	BasicHpp <<
		R"(
template<typename ClassType>
class TSubclassOf
{
	class UClass* ClassPtr;

public:
	TSubclassOf() = default;

	inline TSubclassOf(UClass* Class)
		: ClassPtr(Class)
	{
	}

	inline UClass* Get()
	{
		return ClassPtr;
	}

	inline operator UClass*() const
	{
		return ClassPtr;
	}

	template<typename Target, typename = std::enable_if<std::is_base_of_v<Target, ClassType>, bool>::type>
	inline operator TSubclassOf<Target>() const
	{
		return ClassPtr;
	}

	inline UClass* operator->()
	{
		return ClassPtr;
	}

	inline TSubclassOf& operator=(UClass* Class)
	{
		ClassPtr = Class;

		return *this;
	}

	inline bool operator==(const TSubclassOf& Other) const
	{
		return ClassPtr == Other.ClassPtr;
	}

	inline bool operator!=(const TSubclassOf& Other) const
	{
		return ClassPtr != Other.ClassPtr;
	}

	inline bool operator==(UClass* Other) const
	{
		return ClassPtr == Other;
	}

	inline bool operator!=(UClass* Other) const
	{
		return ClassPtr != Other;
	}
};
)";

	const int32 TextDataSize = (Off::InSDK::Text::InTextDataStringOffset + 0x10);

	/* class FTextData */
	PredefinedStruct FTextData = PredefinedStruct{
		.UniqueName = "FTextData", .Size = TextDataSize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	FTextData.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FString", .Name = "TextSource", .Offset = Off::InSDK::Text::InTextDataStringOffset, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	BasicHpp << R"(namespace FTextImpl
{)";
	GenerateStruct(&FTextData, BasicHpp, BasicCpp, BasicHpp);
	BasicHpp << "}\n";

	/* class FText */
	PredefinedStruct FText = PredefinedStruct{
		.UniqueName = "FText", .Size = Off::InSDK::Text::TextSize, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	FText.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FTextImpl::FTextData*", .Name = "TextData", .Offset = Off::InSDK::Text::TextDatOffset, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	FText.Functions =
	{
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "const class FString&", .NameWithParams = "GetStringRef()", .Body =
R"({
	return TextData->TextSource;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "std::string", .NameWithParams = "ToString()", .Body =
R"({
	return TextData->TextSource.ToString();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
	};

	GenerateStruct(&FText, BasicHpp, BasicCpp, BasicHpp);


	constexpr int32 FWeakObjectPtrSize = 0x8;

	/* class FWeakObjectPtr */
	PredefinedStruct FWeakObjectPtr = PredefinedStruct{
		.UniqueName = "FWeakObjectPtr", .Size = FWeakObjectPtrSize, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	FWeakObjectPtr.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ObjectIndex", .Offset = 0x0, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "ObjectSerialNumber", .Offset = 0x4, .Size = 0x4, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	FWeakObjectPtr.Functions =
	{
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UObject*", .NameWithParams = "Get()", .Body =
R"({
	return UObject::GObjects->GetByIndex(ObjectIndex);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UObject*", .NameWithParams = "operator->()", .Body =
R"({
	return UObject::GObjects->GetByIndex(ObjectIndex);
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator==(const FWeakObjectPtr& Other)", .Body =
R"({
	return ObjectIndex == Other.ObjectIndex;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator!=(const FWeakObjectPtr& Other)", .Body =
R"({
	return ObjectIndex != Other.ObjectIndex;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator==(const class UObject* Other)", .Body =
R"({
	return ObjectIndex == Other->Index;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "bool", .NameWithParams = "operator!=(const class UObject* Other)", .Body =
R"({
	return ObjectIndex != Other->Index;
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = false
		},
	};

	GenerateStruct(&FWeakObjectPtr, BasicHpp, BasicCpp, BasicHpp);


	BasicHpp <<
		R"(
template<typename UEType>
class TWeakObjectPtr : public FWeakObjectPtr
{
public:
	UEType* Get() const
	{
		return static_cast<UEType*>(FWeakObjectPtr::Get());
	}

	UEType* operator->() const
	{
		return static_cast<UEType*>(FWeakObjectPtr::Get());
	}
};
)";


	/* class FUniqueObjectGuid */
	PredefinedStruct FUniqueObjectGuid = PredefinedStruct{
		.UniqueName = "FUniqueObjectGuid", .Size = 0x10, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	FUniqueObjectGuid.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint32", .Name = "A", .Offset = 0x0, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint32", .Name = "B", .Offset = 0x4, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint32", .Name = "C", .Offset = 0x8, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "uint32", .Name = "D", .Offset = 0xC, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	GenerateStruct(&FUniqueObjectGuid, BasicHpp, BasicCpp, BasicHpp);


	/* class TPersistentObjectPtr */
	PredefinedStruct TPersistentObjectPtr = PredefinedStruct{
		.CustomTemplateText = "template<typename TObjectID>",
		.UniqueName = "TPersistentObjectPtr", .Size = 0x0, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	const int32 ObjectIDOffset = Settings::Internal::bIsWeakObjectPtrWithoutTag ? 0x8 : 0xC;

	TPersistentObjectPtr.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "FWeakObjectPtr", .Name = "WeakPtr", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "TagAtLastTest", .Offset = 0x8, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "TObjectID", .Name = "ObjectID", .Offset = ObjectIDOffset, .Size = 0x00, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	if (Settings::Internal::bIsWeakObjectPtrWithoutTag)
		TPersistentObjectPtr.Properties.erase(TPersistentObjectPtr.Properties.begin() + 1); // TagAtLast

	TPersistentObjectPtr.Functions =
	{
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UObject*", .NameWithParams = "Get()", .Body =
R"({
	return WeakPtr.Get();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UObject*", .NameWithParams = "operator->()", .Body =
R"({
	return WeakPtr.Get();
})",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
	};

	GenerateStruct(&TPersistentObjectPtr, BasicHpp, BasicCpp, BasicHpp);


	/* class TLazyObjectPtr */
	BasicHpp <<
		R"(
template<typename UEType>
class TLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid>
{
public:
	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)";


	// Start Namespace 'FakeSoftObjectPtr'
	BasicHpp <<
		R"(
namespace FakeSoftObjectPtr
{
)";

	UEStruct SoftObjectPath = ObjectArray::FindObjectFast<UEStruct>("SoftObjectPath");

	/* if SoftObjectPath doesn't exist just generate FStringAssetReference and call it SoftObjectPath, it's basically the same thing anyways */
	if (!SoftObjectPath)
	{
		/* struct FSoftObjectPtr */
		PredefinedStruct FSoftObjectPath = PredefinedStruct{
			.UniqueName = "FSoftObjectPath", .Size = 0x10, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = false, .bIsUnion = false, .Super = nullptr
		};

		FSoftObjectPath.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class FString", .Name = "AssetLongPathname", .Offset = 0x0, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		GenerateStruct(&FSoftObjectPath, BasicHpp, BasicCpp, BasicHpp);
	}
	else /* if SoftObjectPath exists generate a copy of it within this namespace to allow for TSoftObjectPtr declaration (comes before real SoftObjectPath) */
	{
		UEProperty Assetpath = SoftObjectPath.FindMember("AssetPath");

		if (Assetpath && Assetpath.IsA(EClassCastFlags::StructProperty))
			GenerateStruct(Assetpath.Cast<UEStructProperty>().GetUnderlayingStruct(), BasicHpp, BasicCpp, BasicHpp);

		GenerateStruct(SoftObjectPath, BasicHpp, BasicCpp, BasicHpp);
	}

	// Start Namespace 'FakeSoftObjectPtr'
	BasicHpp << "\n}\n";


	/* struct FSoftObjectPtr */
	BasicHpp <<
		R"(
class FSoftObjectPtr : public TPersistentObjectPtr<FakeSoftObjectPtr::FSoftObjectPath>
{
};
)";


	/* struct TSoftObjectPtr */
	BasicHpp <<
		R"(
template<typename UEType>
class TSoftObjectPtr : public FSoftObjectPtr
{
public:
	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)";

	/* struct TSoftClassPtr */
	BasicHpp <<
		R"(
template<typename UEType>
class TSoftClassPtr : public FSoftObjectPtr
{
public:
	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)";


	/* class FScriptInterface */
	PredefinedStruct FScriptInterface = PredefinedStruct{
		.UniqueName = "FScriptInterface", .Size = 0x10, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	FScriptInterface.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "UObject*", .Name = "ObjectPointer", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "void*", .Name = "InterfacePointer", .Offset = 0x8, .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	FScriptInterface.Functions =
	{
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "class UObject*", .NameWithParams = "GetObjectRef()", .Body =
R"({
	return ObjectPointer;
}
)",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
		PredefinedFunction {
			.CustomComment = "",
			.ReturnType = "void*", .NameWithParams = "GetInterfaceRef()", .Body =
R"({
	return InterfacePointer;
}
)",
			.bIsStatic = false, .bIsConst = true, .bIsBodyInline = true
		},
	};

	GenerateStruct(&FScriptInterface, BasicHpp, BasicCpp, BasicHpp);


	/* class TScriptInterface */
	PredefinedStruct TScriptInterface = PredefinedStruct{
		.CustomTemplateText = "template<class InterfaceType>",
		.UniqueName = "TScriptInterface", .Size = 0x10, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = &FScriptInterface
	};

	GenerateStruct(&TScriptInterface, BasicHpp, BasicCpp, BasicHpp);


	if (Settings::Internal::bUseFProperty)
	{
		/* class FFieldPath */
		PredefinedStruct FFieldPath = PredefinedStruct{
			.UniqueName = "FFieldPath", .Size = PropertySizes::FieldPathProperty, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
		};

		FFieldPath.Properties =
		{
			PredefinedMember {
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class FField*", .Name = "ResolvedField", .Offset = 0x0, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};

		/* #ifdef WITH_EDITORONLY_DATA */
		const bool bIsWithEditorOnlyData = PropertySizes::FieldPathProperty > 0x20;

		if (bIsWithEditorOnlyData)
		{
			FFieldPath.Properties.emplace_back
			(
				PredefinedMember{
					.Comment = "NOT AUTO-GENERATED PROPERTY",
					.Type = "class FFieldClass*", .Name = "InitialFieldClass", .Offset = 0x08, .Size = 0x08, .ArrayDim = 0x1, .Alignment = 0x8,
					.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
				}
			);
			FFieldPath.Properties.emplace_back
			(
				PredefinedMember{
					.Comment = "NOT AUTO-GENERATED PROPERTY",
					.Type = "int32", .Name = "FieldPathSerialNumber", .Offset = 0x10, .Size = 0x04, .ArrayDim = 0x1, .Alignment = 0x4,
					.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
				}
			);
		}

		FFieldPath.Properties.push_back
		(
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "TWeakObjectPtr<class UStruct>", .Name = "ResolvedOwner", .Offset = (bIsWithEditorOnlyData ? 0x14 : 0x8), .Size = 0x8, .ArrayDim = 0x1, .Alignment = 0x4,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
		FFieldPath.Properties.push_back
		(
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "TArray<FName>", .Name = "Path", .Offset = (bIsWithEditorOnlyData ? 0x20 : 0x10), .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);

		GenerateStruct(&FFieldPath, BasicHpp, BasicCpp, BasicHpp);

		/* class TFieldPath */
		PredefinedStruct TFieldPath = PredefinedStruct{
			.CustomTemplateText = "template<class PropertyType>",
			.UniqueName = "TFieldPath", .Size = PropertySizes::FieldPathProperty, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = true, .bIsClass = true, .bIsUnion = false, .Super = &FFieldPath
		};

		GenerateStruct(&TFieldPath, BasicHpp, BasicCpp, BasicHpp);



		// TOptional
		BasicHpp << R"(

template<typename OptionalType, bool bIsIntrusiveUnsetCheck = false>
class TOptional
{
private:
	template<int32 TypeSize>
	struct OptionalWithBool
	{
		static_assert(TypeSize > 0x0, "TOptional can not store an empty type!");

		uint8 Value[TypeSize];
		bool bIsSet;
	};

private:
	using ValueType = std::conditional_t<bIsIntrusiveUnsetCheck, uint8[sizeof(OptionalType)], OptionalWithBool<sizeof(OptionalType)>>;

private:
	alignas(OptionalType) ValueType StoredValue;

private:
	inline uint8* GetValueBytes()
	{
		if constexpr (!bIsIntrusiveUnsetCheck)
			return StoredValue.Value;

		return StoredValue;
	}

	inline const uint8* GetValueBytes() const
	{
		if constexpr (!bIsIntrusiveUnsetCheck)
			return StoredValue.Value;

		return StoredValue;
	}
public:

	inline OptionalType& GetValueRef()
	{
		return *reinterpret_cast<OptionalType*>(GetValueBytes());
	}

	inline const OptionalType& GetValueRef() const
	{
		return *reinterpret_cast<const OptionalType*>(GetValueBytes());
	}

	inline bool IsSet() const
	{
		if constexpr (!bIsIntrusiveUnsetCheck)
			return StoredValue.bIsSet;

		constexpr char ZeroBytes[sizeof(OptionalType)];

		return memcmp(GetValueBytes(), &ZeroBytes, sizeof(OptionalType)) == 0;
	}

	inline explicit operator bool() const
	{
		return IsSet();
	}
};

)";
	} /* End 'if (Settings::Internal::bUseFProperty)' */

	const auto ScriptDelegateSize = (FWeakObjectPtrSize + Off::InSDK::Name::FNameSize);

	/* FScriptDelegate */
	PredefinedStruct FScriptDelegate = PredefinedStruct{
		.UniqueName = "FScriptDelegate", .Size = ScriptDelegateSize, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = false, .bIsUnion = false, .Super = nullptr
	};

	FScriptDelegate.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "FWeakObjectPtr", .Name = "Object", .Offset = 0x0, .Size = FWeakObjectPtrSize, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "FName", .Name = "FunctionName", .Offset = FWeakObjectPtrSize, .Size = Off::InSDK::Name::FNameSize, .ArrayDim = 0x1, .Alignment = 0x4,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	GenerateStruct(&FScriptDelegate, BasicHpp, BasicCpp, BasicHpp);


	/* TDelegate */
	PredefinedStruct TDelegate = PredefinedStruct{
		.CustomTemplateText = "template<typename FunctionSignature>",
		.UniqueName = "TDelegate", .Size = PropertySizes::DelegateProperty, .Alignment = 0x4, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	TDelegate.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "struct InvalidUseOfTDelegate", .Name = "TemplateParamIsNotAFunctionSignature", .Offset = 0x0, .Size = 0x0, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};


	GenerateStruct(&TDelegate, BasicHpp, BasicCpp, BasicHpp);

	/* TDelegate<Ret(Args...)> */
	PredefinedStruct TDelegateSpezialiation = PredefinedStruct{
		.CustomTemplateText = "template<typename Ret, typename... Args>",
		.UniqueName = "TDelegate<Ret(Args...)>", .Size = PropertySizes::DelegateProperty, .Alignment = 0x1, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	TDelegateSpezialiation.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "FScriptDelegate", .Name = "BoundFunction", .Offset = 0x0, .Size = ScriptDelegateSize, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		}
	};

	GenerateStruct(&TDelegateSpezialiation, BasicHpp, BasicCpp, BasicHpp);

	/* TMulticastInlineDelegate */
	PredefinedStruct TMulticastInlineDelegate = PredefinedStruct{
		.CustomTemplateText = "template<typename FunctionSignature>",
		.UniqueName = "TMulticastInlineDelegate", .Size = 0x10, .Alignment = 0x8, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	TMulticastInlineDelegate.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "struct InvalidUseOfTMulticastInlineDelegate", .Name = "TemplateParamIsNotAFunctionSignature", .Offset = 0x0, .Size = ScriptDelegateSize, .ArrayDim = 0x1, .Alignment = 0x1,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	GenerateStruct(&TMulticastInlineDelegate, BasicHpp, BasicCpp, BasicHpp);

	/* TMulticastInlineDelegate<Ret(Args...)> */
	PredefinedStruct TMulticastInlineDelegateSpezialiation = PredefinedStruct{
		.CustomTemplateText = "template<typename Ret, typename... Args>",
		.UniqueName = "TMulticastInlineDelegate<Ret(Args...)>", .Size = 0x0, .Alignment = 0x1, .bUseExplictAlignment = false, .bIsFinal = false, .bIsClass = true, .bIsUnion = false, .Super = nullptr
	};

	TMulticastInlineDelegateSpezialiation.Properties =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "TArray<FScriptDelegate>", .Name = "InvocationList", .Offset = 0x0, .Size = 0x10, .ArrayDim = 0x1, .Alignment = 0x8,
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		}
	};

	GenerateStruct(&TMulticastInlineDelegateSpezialiation, BasicHpp, BasicCpp, BasicHpp);


	/* UE_ENUM_OPERATORS - enum flag operations */
	BasicHpp <<
		R"(
#define UE_ENUM_OPERATORS(EEnumClass)																																	\
																																										\
inline constexpr EEnumClass operator|(EEnumClass Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass)((std::underlying_type<EEnumClass>::type)(Left) | (std::underlying_type<EEnumClass>::type)(Right));												\
}																																										\
																																										\
inline constexpr EEnumClass& operator|=(EEnumClass& Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) |= (std::underlying_type<EEnumClass>::type)(Right));											\
}																																										\
																																										\
inline bool operator&(EEnumClass Left, EEnumClass Right)																												\
{																																										\
	return (((std::underlying_type<EEnumClass>::type)(Left) & (std::underlying_type<EEnumClass>::type)(Right)) == (std::underlying_type<EEnumClass>::type)(Right));		\
}																																										
)";

	/* enum class EObjectFlags */
	BasicHpp <<
		R"(
enum class EObjectFlags : int32
{
	NoFlags							= 0x00000000,

	Public							= 0x00000001,
	Standalone						= 0x00000002,
	MarkAsNative					= 0x00000004,
	Transactional					= 0x00000008,
	ClassDefaultObject				= 0x00000010,
	ArchetypeObject					= 0x00000020,
	Transient						= 0x00000040,

	MarkAsRootSet					= 0x00000080,
	TagGarbageTemp					= 0x00000100,

	NeedInitialization				= 0x00000200,
	NeedLoad						= 0x00000400,
	KeepForCooker					= 0x00000800,
	NeedPostLoad					= 0x00001000,
	NeedPostLoadSubobjects			= 0x00002000,
	NewerVersionExists				= 0x00004000,
	BeginDestroyed					= 0x00008000,
	FinishDestroyed					= 0x00010000,

	BeingRegenerated				= 0x00020000,
	DefaultSubObject				= 0x00040000,
	WasLoaded						= 0x00080000,
	TextExportTransient				= 0x00100000,
	LoadCompleted					= 0x00200000,
	InheritableComponentTemplate	= 0x00400000,
	DuplicateTransient				= 0x00800000,
	StrongRefOnFrame				= 0x01000000,
	NonPIEDuplicateTransient		= 0x02000000,
	Dynamic							= 0x04000000,
	WillBeLoaded					= 0x08000000,
	HasExternalPackage				= 0x10000000,

	MirroredGarbage					= 0x40000000,
	AllocatedInSharedPage			= 0x80000000,
};
)";


	/* enum class EFunctionFlags */
	BasicHpp <<
		R"(
enum class EFunctionFlags : uint32
{
	None							= 0x00000000,

	Final							= 0x00000001,
	RequiredAPI						= 0x00000002,
	BlueprintAuthorityOnly			= 0x00000004, 
	BlueprintCosmetic				= 0x00000008, 
	Net								= 0x00000040,  
	NetReliable						= 0x00000080, 
	NetRequest						= 0x00000100,	
	Exec							= 0x00000200,	
	Native							= 0x00000400,	
	Event							= 0x00000800,   
	NetResponse						= 0x00001000,  
	Static							= 0x00002000,   
	NetMulticast					= 0x00004000,	
	UbergraphFunction				= 0x00008000,  
	MulticastDelegate				= 0x00010000,
	Public							= 0x00020000,	
	Private							= 0x00040000,	
	Protected						= 0x00080000,
	Delegate						= 0x00100000,	
	NetServer						= 0x00200000,	
	HasOutParms						= 0x00400000,	
	HasDefaults						= 0x00800000,
	NetClient						= 0x01000000,
	DLLImport						= 0x02000000,
	BlueprintCallable				= 0x04000000,
	BlueprintEvent					= 0x08000000,
	BlueprintPure					= 0x10000000,	
	EditorOnly						= 0x20000000,	
	Const							= 0x40000000,
	NetValidate						= 0x80000000,

	AllFlags						= 0xFFFFFFFF,
};
)";

	/* enum class EClassFlags */
	BasicHpp <<
		R"(
enum class EClassFlags : uint32
{
	CLASS_None						= 0x00000000u,
	Abstract						= 0x00000001u,
	DefaultConfig					= 0x00000002u,
	Config							= 0x00000004u,
	Transient						= 0x00000008u,
	Parsed							= 0x00000010u,
	MatchedSerializers				= 0x00000020u,
	ProjectUserConfig				= 0x00000040u,
	Native							= 0x00000080u,
	NoExport						= 0x00000100u,
	NotPlaceable					= 0x00000200u,
	PerObjectConfig					= 0x00000400u,
	ReplicationDataIsSetUp			= 0x00000800u,
	EditInlineNew					= 0x00001000u,
	CollapseCategories				= 0x00002000u,
	Interface						= 0x00004000u,
	CustomConstructor				= 0x00008000u,
	Const							= 0x00010000u,
	LayoutChanging					= 0x00020000u,
	CompiledFromBlueprint			= 0x00040000u,
	MinimalAPI						= 0x00080000u,
	RequiredAPI						= 0x00100000u,
	DefaultToInstanced				= 0x00200000u,
	TokenStreamAssembled			= 0x00400000u,
	HasInstancedReference			= 0x00800000u,
	Hidden							= 0x01000000u,
	Deprecated						= 0x02000000u,
	HideDropDown					= 0x04000000u,
	GlobalUserConfig				= 0x08000000u,
	Intrinsic						= 0x10000000u,
	Constructed						= 0x20000000u,
	ConfigDoNotCheckDefaults		= 0x40000000u,
	NewerVersionExists				= 0x80000000u,
};
)";

	/* enum class EClassCastFlags */
	BasicHpp <<
		R"(
enum class EClassCastFlags : uint64
{
	None = 0x0000000000000000,

	Field								= 0x0000000000000001,
	Int8Property						= 0x0000000000000002,
	Enum								= 0x0000000000000004,
	Struct								= 0x0000000000000008,
	ScriptStruct						= 0x0000000000000010,
	Class								= 0x0000000000000020,
	ByteProperty						= 0x0000000000000040,
	IntProperty							= 0x0000000000000080,
	FloatProperty						= 0x0000000000000100,
	UInt64Property						= 0x0000000000000200,
	ClassProperty						= 0x0000000000000400,
	UInt32Property						= 0x0000000000000800,
	InterfaceProperty					= 0x0000000000001000,
	NameProperty						= 0x0000000000002000,
	StrProperty							= 0x0000000000004000,
	Property							= 0x0000000000008000,
	ObjectProperty						= 0x0000000000010000,
	BoolProperty						= 0x0000000000020000,
	UInt16Property						= 0x0000000000040000,
	Function							= 0x0000000000080000,
	StructProperty						= 0x0000000000100000,
	ArrayProperty						= 0x0000000000200000,
	Int64Property						= 0x0000000000400000,
	DelegateProperty					= 0x0000000000800000,
	NumericProperty						= 0x0000000001000000,
	MulticastDelegateProperty			= 0x0000000002000000,
	ObjectPropertyBase					= 0x0000000004000000,
	WeakObjectProperty					= 0x0000000008000000,
	LazyObjectProperty					= 0x0000000010000000,
	SoftObjectProperty					= 0x0000000020000000,
	TextProperty						= 0x0000000040000000,
	Int16Property						= 0x0000000080000000,
	DoubleProperty						= 0x0000000100000000,
	SoftClassProperty					= 0x0000000200000000,
	Package								= 0x0000000400000000,
	Level								= 0x0000000800000000,
	Actor								= 0x0000001000000000,
	PlayerController					= 0x0000002000000000,
	Pawn								= 0x0000004000000000,
	SceneComponent						= 0x0000008000000000,
	PrimitiveComponent					= 0x0000010000000000,
	SkinnedMeshComponent				= 0x0000020000000000,
	SkeletalMeshComponent				= 0x0000040000000000,
	Blueprint							= 0x0000080000000000,
	DelegateFunction					= 0x0000100000000000,
	StaticMeshComponent					= 0x0000200000000000,
	MapProperty							= 0x0000400000000000,
	SetProperty							= 0x0000800000000000,
	EnumProperty						= 0x0001000000000000,
	USparseDelegateFunction				= 0x0002000000000000,
	MulticastInlineDelegateProperty	    = 0x0004000000000000,
	MulticastSparseDelegateProperty	    = 0x0008000000000000,
	FieldPathProperty					= 0x0010000000000000,
	LargeWorldCoordinatesRealProperty	= 0x0080000000000000,
	OptionalProperty					= 0x0100000000000000,
	VValueProperty						= 0x0200000000000000,
	UVerseVMClass						= 0x0400000000000000,
	VRestValueProperty					= 0x0800000000000000,
};
)";

	/* enum class EPropertyFlags */
	BasicHpp <<
		R"(
enum class EPropertyFlags : uint64
{
	None								= 0x0000000000000000,

	Edit								= 0x0000000000000001,
	ConstParm							= 0x0000000000000002,
	BlueprintVisible					= 0x0000000000000004,
	ExportObject						= 0x0000000000000008,
	BlueprintReadOnly					= 0x0000000000000010,
	Net									= 0x0000000000000020,
	EditFixedSize						= 0x0000000000000040,
	Parm								= 0x0000000000000080,
	OutParm								= 0x0000000000000100,
	ZeroConstructor						= 0x0000000000000200,
	ReturnParm							= 0x0000000000000400,
	DisableEditOnTemplate 				= 0x0000000000000800,

	Transient							= 0x0000000000002000,
	Config								= 0x0000000000004000,

	DisableEditOnInstance				= 0x0000000000010000,
	EditConst							= 0x0000000000020000,
	GlobalConfig						= 0x0000000000040000,
	InstancedReference					= 0x0000000000080000,	

	DuplicateTransient					= 0x0000000000200000,	
	SubobjectReference					= 0x0000000000400000,	

	SaveGame							= 0x0000000001000000,
	NoClear								= 0x0000000002000000,

	ReferenceParm						= 0x0000000008000000,
	BlueprintAssignable					= 0x0000000010000000,
	Deprecated							= 0x0000000020000000,
	IsPlainOldData						= 0x0000000040000000,
	RepSkip								= 0x0000000080000000,
	RepNotify							= 0x0000000100000000, 
	Interp								= 0x0000000200000000,
	NonTransactional					= 0x0000000400000000,
	EditorOnly							= 0x0000000800000000,
	NoDestructor						= 0x0000001000000000,

	AutoWeak							= 0x0000004000000000,
	ContainsInstancedReference			= 0x0000008000000000,	
	AssetRegistrySearchable				= 0x0000010000000000,
	SimpleDisplay						= 0x0000020000000000,
	AdvancedDisplay						= 0x0000040000000000,
	Protected							= 0x0000080000000000,
	BlueprintCallable					= 0x0000100000000000,
	BlueprintAuthorityOnly				= 0x0000200000000000,
	TextExportTransient					= 0x0000400000000000,
	NonPIEDuplicateTransient			= 0x0000800000000000,
	ExposeOnSpawn						= 0x0001000000000000,
	PersistentInstance					= 0x0002000000000000,
	UObjectWrapper						= 0x0004000000000000, 
	HasGetValueTypeHash					= 0x0008000000000000, 
	NativeAccessSpecifierPublic			= 0x0010000000000000,	
	NativeAccessSpecifierProtected		= 0x0020000000000000,
	NativeAccessSpecifierPrivate		= 0x0040000000000000,	
	SkipSerialization					= 0x0080000000000000, 
};
)";

	/* enum class EClassCastFlags */
	BasicHpp << R"(
UE_ENUM_OPERATORS(EObjectFlags);
UE_ENUM_OPERATORS(EFunctionFlags);
UE_ENUM_OPERATORS(EClassFlags);
UE_ENUM_OPERATORS(EClassCastFlags);
UE_ENUM_OPERATORS(EPropertyFlags);
)";



	/* Write Predefined Structs into Basic.hpp */
	for (const PredefinedStruct& Predefined : PredefinedStructs)
	{
		GenerateStruct(&Predefined, BasicHpp, BasicCpp, BasicHpp);
	}


	/* Cyclic dependencies-fixing helper classes */

	// Start Namespace 'CyclicDependencyFixupImpl'
	BasicHpp <<
		R"(
namespace CyclicDependencyFixupImpl
{
)";

	/*
	* Implemenation node:
	*	Maybe: when inheriting form TCylicStructFixup/TCyclicClassFixup use the aligned size, else use UnalignedSize
	*/

	/* TStructOrderFixup */
	BasicHpp << R"(
/*
* A wrapper for a Byte-Array of padding, that allows for casting to the actual underlaiyng type. Used for undefined structs in cylic headers.
*/
template<typename UnderlayingStructType, int32 Size, int32 Align>
struct alignas(Align) TCylicStructFixup
{
private:
	uint8 Pad[Size];

public:
	      UnderlayingStructType& GetTyped()       { return reinterpret_cast<      UnderlayingStructType&>(*this); }
	const UnderlayingStructType& GetTyped() const { return reinterpret_cast<const UnderlayingStructType&>(*this); }
};
)";
	BasicHpp << R"(
/*
* A wrapper for a Byte-Array of padding, that inherited from UObject allows for casting to the actual underlaiyng type and access to basic UObject functionality. For cyclic classes.
*/
template<typename UnderlayingClassType, int32 Size, int32 Align = 0x8, class BaseClassType = class UObject>
struct alignas(Align) TCyclicClassFixup : public BaseClassType
{
private:
	uint8 Pad[Size];

public:
	UnderlayingClassType*       GetTyped()       { return reinterpret_cast<      UnderlayingClassType*>(this); }
	const UnderlayingClassType* GetTyped() const { return reinterpret_cast<const UnderlayingClassType*>(this); }
};

)";

	BasicHpp << "}\n\n";
	// End Namespace 'CyclicDependencyFixupImpl'


	BasicHpp << R"(
template<typename UnderlayingStructType, int32 Size, int32 Align>
using TStructCycleFixup = CyclicDependencyFixupImpl::TCylicStructFixup<UnderlayingStructType, Size, Align>;


template<typename UnderlayingClassType, int32 Size, int32 Align = 0x8>
using TObjectBasedCycleFixup = CyclicDependencyFixupImpl::TCyclicClassFixup<UnderlayingClassType, Size, Align, class UObject>;

template<typename UnderlayingClassType, int32 Size, int32 Align = 0x8>
using TActorBasedCycleFixup = CyclicDependencyFixupImpl::TCyclicClassFixup<UnderlayingClassType, Size, Align, class AActor>;
)";


	WriteFileEnd(BasicHpp, EFileType::BasicHpp);
	WriteFileEnd(BasicCpp, EFileType::BasicCpp);
}


/* See https://github.com/Fischsalat/UnrealContainers/blob/master/UnrealContainers/UnrealContainersNoAlloc.h */
void CppGenerator::GenerateUnrealContainers(StreamType& UEContainersHeader)
{
	WriteFileHead(UEContainersHeader, nullptr, EFileType::UnrealContainers, 
		"Container implementations with iterators. See https://github.com/Fischsalat/UnrealContainers", "#include <string>\n#include <stdexcept>\n#include <iostream>\n#include \"UtfN.hpp\"");


	UEContainersHeader << R"(
namespace UC
{	
	typedef int8_t  int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef int64_t int64;

	typedef uint8_t  uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;

	template<typename ArrayElementType>
	class TArray;

	template<typename SparseArrayElementType>
	class TSparseArray;

	template<typename SetElementType>
	class TSet;

	template<typename KeyElementType, typename ValueElementType>
	class TMap;

	template<typename KeyElementType, typename ValueElementType>
	class TPair;

	namespace Iterators
	{
		class FSetBitIterator;

		template<typename ArrayType>
		class TArrayIterator;

		template<class ContainerType>
		class TContainerIterator;

		template<typename SparseArrayElementType>
		using TSparseArrayIterator = TContainerIterator<TSparseArray<SparseArrayElementType>>;

		template<typename SetElementType>
		using TSetIterator = TContainerIterator<TSet<SetElementType>>;

		template<typename KeyElementType, typename ValueElementType>
		using TMapIterator = TContainerIterator<TMap<KeyElementType, ValueElementType>>;
	}


	namespace ContainerImpl
	{
		namespace HelperFunctions
		{
			inline uint32 FloorLog2(uint32 Value)
			{
				uint32 pos = 0;
				if (Value >= 1 << 16) { Value >>= 16; pos += 16; }
				if (Value >= 1 << 8) { Value >>= 8; pos += 8; }
				if (Value >= 1 << 4) { Value >>= 4; pos += 4; }
				if (Value >= 1 << 2) { Value >>= 2; pos += 2; }
				if (Value >= 1 << 1) { pos += 1; }
				return pos;
			}

			inline uint32 CountLeadingZeros(uint32 Value)
			{
				if (Value == 0)
					return 32;

				return 31 - FloorLog2(Value);
			}
		}

		template<int32 Size, uint32 Alignment>
		struct TAlignedBytes
		{
			alignas(Alignment) uint8 Pad[Size];
		};

		template<uint32 NumInlineElements>
		class TInlineAllocator
		{
		public:
			template<typename ElementType>
			class ForElementType
			{
			private:
				static constexpr int32 ElementSize = sizeof(ElementType);
				static constexpr int32 ElementAlign = alignof(ElementType);

				static constexpr int32 InlineDataSizeBytes = NumInlineElements * ElementSize;

			private:
				TAlignedBytes<ElementSize, ElementAlign> InlineData[NumInlineElements];
				ElementType* SecondaryData;

			public:
				ForElementType()
					: InlineData{ 0x0 }, SecondaryData(nullptr)
				{
				}

				ForElementType(ForElementType&&) = default;
				ForElementType(const ForElementType&) = default;

			public:
				ForElementType& operator=(ForElementType&&) = default;
				ForElementType& operator=(const ForElementType&) = default;

			public:
				inline const ElementType* GetAllocation() const { return SecondaryData ? SecondaryData : reinterpret_cast<const ElementType*>(&InlineData); }

				inline uint32 GetNumInlineBytes() const { return NumInlineElements; }
			};
		};

		class FBitArray
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		private:
			TInlineAllocator<4>::ForElementType<int32> Data;
			int32 NumBits;
			int32 MaxBits;

		public:
			FBitArray()
				: NumBits(0), MaxBits(Data.GetNumInlineBytes() * NumBitsPerDWORD)
			{
			}

			FBitArray(const FBitArray&) = default;

			FBitArray(FBitArray&&) = default;

		public:
			FBitArray& operator=(FBitArray&&) = default;

			FBitArray& operator=(const FBitArray& Other) = default;

		private:
			inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		public:
			inline int32 Num() const { return NumBits; }
			inline int32 Max() const { return MaxBits; }

			inline const uint32* GetData() const { return reinterpret_cast<const uint32*>(Data.GetAllocation()); }

			inline bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < NumBits; }

			inline bool IsValid() const { return GetData() && NumBits > 0; }

		public:
			inline bool operator[](int32 Index) const { VerifyIndex(Index); return GetData()[Index / NumBitsPerDWORD] & (1 << (Index & (NumBitsPerDWORD - 1))); }

			inline bool operator==(const FBitArray& Other) const { return NumBits == Other.NumBits && GetData() == Other.GetData(); }
			inline bool operator!=(const FBitArray& Other) const { return NumBits != Other.NumBits || GetData() != Other.GetData(); }

		public:
			friend Iterators::FSetBitIterator begin(const FBitArray& Array);
			friend Iterators::FSetBitIterator end  (const FBitArray& Array);
		};

		template<typename SparseArrayType>
		union TSparseArrayElementOrFreeListLink
		{
			SparseArrayType ElementData;

			struct
			{
				int32 PrevFreeIndex;
				int32 NextFreeIndex;
			};
		};

		template<typename SetType>
		class SetElement
		{
		private:
			template<typename SetDataType>
			friend class TSet;

		private:
			SetType Value;
			int32 HashNextId;
			int32 HashIndex;
		};
	}


	template <typename KeyType, typename ValueType>
	class TPair
	{
	public:
		KeyType First;
		ValueType Second;

	public:
		TPair(KeyType Key, ValueType Value)
			: First(Key), Second(Value)
		{
		}

	public:
		inline       KeyType& Key()       { return First; }
		inline const KeyType& Key() const { return First; }

		inline       ValueType& Value()       { return Second; }
		inline const ValueType& Value() const { return Second; }
	};

	template<typename ArrayElementType>
	class TArray
	{
	private:
		template<typename ArrayElementType>
		friend class TAllocatedArray;

		template<typename SparseArrayElementType>
		friend class TSparseArray;

	protected:
		static constexpr uint64 ElementAlign = alignof(ArrayElementType);
		static constexpr uint64 ElementSize = sizeof(ArrayElementType);

	protected:
		ArrayElementType* Data;
		int32 NumElements;
		int32 MaxElements;

	public:
		TArray()
			: Data(nullptr), NumElements(0), MaxElements(0)
		{
		}

		TArray(const TArray&) = default;

		TArray(TArray&&) = default;

	public:
		TArray& operator=(TArray&&) = default;
		TArray& operator=(const TArray&) = default;

	private:
		inline int32 GetSlack() const { return MaxElements - NumElements; }

		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		inline       ArrayElementType& GetUnsafe(int32 Index)       { return Data[Index]; }
		inline const ArrayElementType& GetUnsafe(int32 Index) const { return Data[Index]; }

	public:
		/* Adds to the array if there is still space for one more element */
		inline bool Add(const ArrayElementType& Element)
		{
			if (GetSlack() <= 0)
				return false;

			Data[NumElements] = Element;
			NumElements++;

			return true;
		}

		inline bool Remove(int32 Index)
		{
			if (!IsValidIndex(Index))
				return false;

			NumElements--;

			for (int i = Index; i < NumElements; i++)
			{
				/* NumElements was decremented, acessing i + 1 is safe */
				Data[i] = Data[i + 1];
			}

			return true;
		}

		inline void Clear()
		{
			NumElements = 0;

			if (Data)
				memset(Data, 0, NumElements * ElementSize);
		}

	public:
		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return MaxElements; }

		inline const ArrayElementType* GetDataPtr() const { return Data; }

		inline bool IsValidIndex(int32 Index) const { return Data && Index >= 0 && Index < NumElements; }

		inline bool IsValid() const { return Data && NumElements > 0 && MaxElements >= NumElements; }

	public:
		inline       ArrayElementType& operator[](int32 Index)       { VerifyIndex(Index); return Data[Index]; }
		inline const ArrayElementType& operator[](int32 Index) const { VerifyIndex(Index); return Data[Index]; }

		inline bool operator==(const TArray<ArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<ArrayElementType>& Other) const { return Data != Other.Data; }

		inline explicit operator bool() const { return IsValid(); };

	public:
		template<typename T> friend Iterators::TArrayIterator<T> begin(const TArray& Array);
		template<typename T> friend Iterators::TArrayIterator<T> end  (const TArray& Array);
	};

	class FString : public TArray<wchar_t>
	{
	public:
		friend std::ostream& operator<<(std::ostream& Stream, const UC::FString& Str) { return Stream << Str.ToString(); }

	public:
		using TArray::TArray;

		FString(const wchar_t* Str)
		{
			const uint32 NullTerminatedLength = static_cast<uint32>(wcslen(Str) + 0x1);

			Data = const_cast<wchar_t*>(Str);
			NumElements = NullTerminatedLength;
			MaxElements = NullTerminatedLength;
		}

	public:
		inline std::string ToString() const
		{
			if (*this)
			{
				return UtfN::Utf16StringToUtf8String<std::string>(Data, NumElements  - 1); // Exclude null-terminator
			}

			return "";
		}

		inline std::wstring ToWString() const
		{
			if (*this)
				return std::wstring(Data);

			return L"";
		}

	public:
		inline       wchar_t* CStr()       { return Data; }
		inline const wchar_t* CStr() const { return Data; }

	public:
		inline bool operator==(const FString& Other) const { return Other ? NumElements == Other.NumElements && wcscmp(Data, Other.Data) == 0 : false; }
		inline bool operator!=(const FString& Other) const { return Other ? NumElements != Other.NumElements || wcscmp(Data, Other.Data) != 0 : true; }
	};

	/*
	* Class to allow construction of a TArray, that uses c-style standard-library memory allocation.
	* 
	* Useful for calling functions that expect a buffer of a certain size and do not reallocate that buffer.
	* This avoids leaking memory, if the array would otherwise be allocated by the engine, and couldn't be freed without FMemory-functions.
	*/
	template<typename ArrayElementType>
	class TAllocatedArray : public TArray<ArrayElementType>
	{
	public:
		TAllocatedArray() = delete;

	public:
		TAllocatedArray(int32 Size)
		{
			this->Data = static_cast<ArrayElementType*>(malloc(Size * sizeof(ArrayElementType)));
			this->NumElements = 0x0;
			this->MaxElements = Size;
		}

		~TAllocatedArray()
		{
			if (this->Data)
				free(this->Data);

			this->NumElements = 0x0;
			this->MaxElements = 0x0;
		}

	public:
		inline operator       TArray<ArrayElementType>()       { return *reinterpret_cast<      TArray<ArrayElementType>*>(this); }
		inline operator const TArray<ArrayElementType>() const { return *reinterpret_cast<const TArray<ArrayElementType>*>(this); }
	};

	/*
	* Class to allow construction of an FString, that uses c-style standard-library memory allocation.
	*
	* Useful for calling functions that expect a buffer of a certain size and do not reallocate that buffer.
	* This avoids leaking memory, if the array would otherwise be allocated by the engine, and couldn't be freed without FMemory-functions.
	*/
	class FAllocatedString : public FString
	{
	public:
		FAllocatedString() = delete;

	public:
		FAllocatedString(int32 Size)
		{
			Data = static_cast<wchar_t*>(malloc(Size * sizeof(wchar_t)));
			NumElements = 0x0;
			MaxElements = Size;
		}

		~FAllocatedString()
		{
			if (Data)
				free(Data);

			NumElements = 0x0;
			MaxElements = 0x0;
		}

	public:
		inline operator       FString()       { return *reinterpret_cast<      FString*>(this); }
		inline operator const FString() const { return *reinterpret_cast<const FString*>(this); }
	};)";

	UEContainersHeader << R"(
	template<typename SparseArrayElementType>
	class TSparseArray
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SparseArrayElementType);
		static constexpr uint32 ElementSize = sizeof(SparseArrayElementType);

	private:
		using FElementOrFreeListLink = ContainerImpl::TSparseArrayElementOrFreeListLink<ContainerImpl::TAlignedBytes<ElementSize, ElementAlign>>;

	private:
		TArray<FElementOrFreeListLink> Data;
		ContainerImpl::FBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		TSparseArray()
			: FirstFreeIndex(-1), NumFreeIndices(0)
		{
		}

		TSparseArray(TSparseArray&&) = default;
		TSparseArray(const TSparseArray&) = default;

	public:
		TSparseArray& operator=(TSparseArray&&) = default;
		TSparseArray& operator=(const TSparseArray&) = default;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 NumAllocated() const { return Data.Num(); }

		inline int32 Num() const { return NumAllocated() - NumFreeIndices; }
		inline int32 Max() const { return Data.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Data.IsValidIndex(Index) && AllocationFlags[Index]; }

		inline bool IsValid() const { return Data.IsValid() && AllocationFlags.IsValid(); }

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return AllocationFlags; }

	public:
		inline       SparseArrayElementType& operator[](int32 Index)       { VerifyIndex(Index); return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData); }
		inline const SparseArrayElementType& operator[](int32 Index) const { VerifyIndex(Index); return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData); }

		inline bool operator==(const TSparseArray<SparseArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TSparseArray<SparseArrayElementType>& Other) const { return Data != Other.Data; }

	public:
		template<typename T> friend Iterators::TSparseArrayIterator<T> begin(const TSparseArray& Array);
		template<typename T> friend Iterators::TSparseArrayIterator<T> end  (const TSparseArray& Array);
	};

	template<typename SetElementType>
	class TSet
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SetElementType);
		static constexpr uint32 ElementSize = sizeof(SetElementType);

	private:
		using SetDataType = ContainerImpl::SetElement<SetElementType>;
		using HashType = ContainerImpl::TInlineAllocator<1>::ForElementType<int32>;

	private:
		TSparseArray<SetDataType> Elements;
		HashType Hash;
		int32 HashSize;

	public:
		TSet()
			: HashSize(0)
		{
		}

		TSet(TSet&&) = default;
		TSet(const TSet&) = default;

	public:
		TSet& operator=(TSet&&) = default;
		TSet& operator=(const TSet&) = default;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 NumAllocated() const { return Elements.NumAllocated(); }

		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return Elements.GetAllocationFlags(); }

	public:
		inline       SetElementType& operator[] (int32 Index)       { return Elements[Index].Value; }
		inline const SetElementType& operator[] (int32 Index) const { return Elements[Index].Value; }

		inline bool operator==(const TSet<SetElementType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TSet<SetElementType>& Other) const { return Elements != Other.Elements; }

	public:
		template<typename T> friend Iterators::TSetIterator<T> begin(const TSet& Set);
		template<typename T> friend Iterators::TSetIterator<T> end  (const TSet& Set);
	};

	template<typename KeyElementType, typename ValueElementType>
	class TMap
	{
	public:
		using ElementType = TPair<KeyElementType, ValueElementType>;

	private:
		TSet<ElementType> Elements;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 NumAllocated() const { return Elements.NumAllocated(); }

		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return Elements.GetAllocationFlags(); }

	public:
		inline decltype(auto) Find(const KeyElementType& Key, bool(*Equals)(const KeyElementType& LeftKey, const KeyElementType& RightKey))
		{
			for (auto It = begin(*this); It != end(*this); ++It)
			{
				if (Equals(It->Key(), Key))
					return It;
			}
		
			return end(*this);
		}

	public:
		inline       ElementType& operator[] (int32 Index)       { return Elements[Index]; }
		inline const ElementType& operator[] (int32 Index) const { return Elements[Index]; }

		inline bool operator==(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements != Other.Elements; }

	public:
		template<typename KeyType, typename ValueType> friend Iterators::TMapIterator<KeyType, ValueType> begin(const TMap& Map);
		template<typename KeyType, typename ValueType> friend Iterators::TMapIterator<KeyType, ValueType> end  (const TMap& Map);
	};

	namespace Iterators
	{
		class FRelativeBitReference
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		public:
			inline explicit FRelativeBitReference(int32 BitIndex)
				: WordIndex(BitIndex >> NumBitsPerDWORDLogTwo)
				, Mask(1 << (BitIndex & (NumBitsPerDWORD - 1)))
			{
			}

			int32  WordIndex;
			uint32 Mask;
		};

		class FSetBitIterator : public FRelativeBitReference
		{
		private:
			const ContainerImpl::FBitArray& Array;

			uint32 UnvisitedBitMask;
			int32 CurrentBitIndex;
			int32 BaseBitIndex;

		public:
			explicit FSetBitIterator(const ContainerImpl::FBitArray& InArray, int32 StartIndex = 0)
				: FRelativeBitReference(StartIndex)
				, Array(InArray)
				, UnvisitedBitMask((~0U) << (StartIndex & (NumBitsPerDWORD - 1)))
				, CurrentBitIndex(StartIndex)
				, BaseBitIndex(StartIndex & ~(NumBitsPerDWORD - 1))
			{
				if (StartIndex != Array.Num())
					FindFirstSetBit();
			}

		public:
			inline FSetBitIterator& operator++()
			{
				UnvisitedBitMask &= ~this->Mask;

				FindFirstSetBit();

				return *this;
			}

			inline explicit operator bool() const { return CurrentBitIndex < Array.Num(); }

			inline bool operator==(const FSetBitIterator& Rhs) const { return CurrentBitIndex == Rhs.CurrentBitIndex && &Array == &Rhs.Array; }
			inline bool operator!=(const FSetBitIterator& Rhs) const { return CurrentBitIndex != Rhs.CurrentBitIndex || &Array != &Rhs.Array; }

		public:
			inline int32 GetIndex() { return CurrentBitIndex; }

			void FindFirstSetBit()
			{
				const uint32* ArrayData = Array.GetData();
				const int32   ArrayNum = Array.Num();
				const int32   LastWordIndex = (ArrayNum - 1) / NumBitsPerDWORD;

				uint32 RemainingBitMask = ArrayData[this->WordIndex] & UnvisitedBitMask;
				while (!RemainingBitMask)
				{
					++this->WordIndex;
					BaseBitIndex += NumBitsPerDWORD;
					if (this->WordIndex > LastWordIndex)
					{
						CurrentBitIndex = ArrayNum;
						return;
					}

					RemainingBitMask = ArrayData[this->WordIndex];
					UnvisitedBitMask = ~0;
				}

				const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

				this->Mask = NewRemainingBitMask ^ RemainingBitMask;

				CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - ContainerImpl::HelperFunctions::CountLeadingZeros(this->Mask);

				if (CurrentBitIndex > ArrayNum)
					CurrentBitIndex = ArrayNum;
			}
		};

		template<typename ArrayType>
		class TArrayIterator
		{
		private:
			TArray<ArrayType>& IteratedArray;
			int32 Index;

		public:
			TArrayIterator(const TArray<ArrayType>& Array, int32 StartIndex = 0x0)
				: IteratedArray(const_cast<TArray<ArrayType>&>(Array)), Index(StartIndex)
			{
			}

		public:
			inline int32 GetIndex() { return Index; }

			inline int32 IsValid() { return IteratedArray.IsValidIndex(GetIndex()); }

		public:
			inline TArrayIterator& operator++() { ++Index; return *this; }
			inline TArrayIterator& operator--() { --Index; return *this; }

			inline       ArrayType& operator*()       { return IteratedArray[GetIndex()]; }
			inline const ArrayType& operator*() const { return IteratedArray[GetIndex()]; }

			inline       ArrayType* operator->()       { return &IteratedArray[GetIndex()]; }
			inline const ArrayType* operator->() const { return &IteratedArray[GetIndex()]; }

			inline bool operator==(const TArrayIterator& Other) const { return &IteratedArray == &Other.IteratedArray && Index == Other.Index; }
			inline bool operator!=(const TArrayIterator& Other) const { return &IteratedArray != &Other.IteratedArray || Index != Other.Index; }
		};

		template<class ContainerType>
		class TContainerIterator
		{
		private:
			ContainerType& IteratedContainer;
			FSetBitIterator BitIterator;

		public:
			TContainerIterator(const ContainerType& Container, const ContainerImpl::FBitArray& BitArray, int32 StartIndex = 0x0)
				: IteratedContainer(const_cast<ContainerType&>(Container)), BitIterator(BitArray, StartIndex)
			{
			}

		public:
			inline int32 GetIndex() { return BitIterator.GetIndex(); }

			inline int32 IsValid() { return IteratedContainer.IsValidIndex(GetIndex()); }

		public:
			inline TContainerIterator& operator++() { ++BitIterator; return *this; }
			inline TContainerIterator& operator--() { --BitIterator; return *this; }

			inline       auto& operator*()       { return IteratedContainer[GetIndex()]; }
			inline const auto& operator*() const { return IteratedContainer[GetIndex()]; }

			inline       auto* operator->()       { return &IteratedContainer[GetIndex()]; }
			inline const auto* operator->() const { return &IteratedContainer[GetIndex()]; }

			inline bool operator==(const TContainerIterator& Other) const { return &IteratedContainer == &Other.IteratedContainer && BitIterator == Other.BitIterator; }
			inline bool operator!=(const TContainerIterator& Other) const { return &IteratedContainer != &Other.IteratedContainer || BitIterator != Other.BitIterator; }
		};
	}

	inline Iterators::FSetBitIterator begin(const ContainerImpl::FBitArray& Array) { return Iterators::FSetBitIterator(Array, 0); }
	inline Iterators::FSetBitIterator end  (const ContainerImpl::FBitArray& Array) { return Iterators::FSetBitIterator(Array, Array.Num()); }

	template<typename T> inline Iterators::TArrayIterator<T> begin(const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, 0); }
	template<typename T> inline Iterators::TArrayIterator<T> end  (const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, Array.Num()); }

	template<typename T> inline Iterators::TSparseArrayIterator<T> begin(const TSparseArray<T>& Array) { return Iterators::TSparseArrayIterator<T>(Array, Array.GetAllocationFlags(), 0); }
	template<typename T> inline Iterators::TSparseArrayIterator<T> end  (const TSparseArray<T>& Array) { return Iterators::TSparseArrayIterator<T>(Array, Array.GetAllocationFlags(), Array.NumAllocated()); }

	template<typename T> inline Iterators::TSetIterator<T> begin(const TSet<T>& Set) { return Iterators::TSetIterator<T>(Set, Set.GetAllocationFlags(), 0); }
	template<typename T> inline Iterators::TSetIterator<T> end  (const TSet<T>& Set) { return Iterators::TSetIterator<T>(Set, Set.GetAllocationFlags(), Set.NumAllocated()); }

	template<typename T0, typename T1> inline Iterators::TMapIterator<T0, T1> begin(const TMap<T0, T1>& Map) { return Iterators::TMapIterator<T0, T1>(Map, Map.GetAllocationFlags(), 0); }
	template<typename T0, typename T1> inline Iterators::TMapIterator<T0, T1> end  (const TMap<T0, T1>& Map) { return Iterators::TMapIterator<T0, T1>(Map, Map.GetAllocationFlags(), Map.NumAllocated()); }

	static_assert(sizeof(TArray<int32>) == 0x10, "TArray has a wrong size!");
	static_assert(sizeof(TSet<int32>) == 0x50, "TSet has a wrong size!");
	static_assert(sizeof(TMap<int32, int32>) == 0x50, "TMap has a wrong size!");
}
)";


	WriteFileEnd(UEContainersHeader, EFileType::UnrealContainers);
}

/* See https://github.com/Fischsalat/UTF-N */
void CppGenerator::GenerateUnicodeLib(StreamType& UnicodeLib) {
	WriteFileHead(UnicodeLib, nullptr, EFileType::UnicodeLib,
		"A simple C++ lib for converting between Utf8, Utf16 and Utf32. See https://github.com/Fischsalat/UnrealContainers");

	UnicodeLib << R"(
// Lower warning-level and turn off certain warnings for STL compilation
#if (defined(_MSC_VER))
#pragma warning (push, 2) // Push warnings and set warn-level to 2
#pragma warning(disable : 4365) // signed/unsigned mismatch
#pragma warning(disable : 4710) // 'FunctionName' was not inlined
#pragma warning(disable : 4711) // 'FunctionName' selected for automatic inline expansion
#elif (defined(__CLANG__) || defined(__GNUC__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include <string>
#include <limits>
#include <cstdint>
#include <type_traits>

#ifdef _DEBUG
#include <stdexcept>
#endif // _DEBUG


// Restore warnings-levels after STL includes
#if (defined(_MSC_VER))
#pragma warning (pop)
#elif (defined(__CLANG__) || defined(__GNUC__))
#pragma GCC diagnostic pop
#endif // Warnings



#if (defined(_MSC_VER))
#pragma warning (push)
#pragma warning (disable: 4514) // C4514 "unreferenced inline function has been removed"
#pragma warning (disable: 4820) // C4820 "'n' bytes padding added after data member '...'"
#pragma warning(disable : 4127) // C4127 conditional expression is constant
#pragma warning(disable : 5045) // C5045 Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#pragma warning(disable : 5246) // C5246 'ArrayVariable' the initialization of a subobject should be wrapped in braces
#elif (defined(__CLANG__) || defined(__GNUC__))
#endif // Warnings

#ifdef __cpp_constexpr
#define UTF_CONSTEXPR constexpr
#else
#define UTF_CONSTEXPR
#endif // __cpp_constexpr


#ifdef __cpp_if_constexpr
#define UTF_IF_CONSTEXPR constexpr
#else
#define UTF_IF_CONSTEXPR
#endif // __cpp_if_constexpr


#if (defined(__cpp_constexpr) && __cpp_constexpr >= 201304L)
#define UTF_CONSTEXPR14 constexpr
#else 
#define UTF_CONSTEXPR14
#endif

#if (defined(__cpp_constexpr) && __cpp_constexpr >= 201603L)
#define UTF_CONSTEXPR17 constexpr
#else 
#define UTF_CONSTEXPR17 inline
#endif

#if (defined(__cpp_constexpr) && __cpp_constexpr >= 201907L)
#define UTF_CONSTEXPR20 constexpr
#else 
#define UTF_CONSTEXPR20 inline
#endif

#if (defined(__cpp_constexpr) && __cpp_constexpr >= 202211L)
#define UTF_CONSTEXPR23 constexpr
#else 
#define UTF_CONSTEXPR23 inline
#endif

#if (defined(__cpp_constexpr) && __cpp_constexpr >= 202406L)
#define UTF_CONSTEXPR26 constexpr
#else 
#define UTF_CONSTEXPR26 inline
#endif


#ifdef __cpp_nodiscard
#define UTF_NODISCARD [[nodiscard]]
#else
#define UTF_NODISCARD
#endif


namespace UtfN
{
#if defined(__cpp_char8_t)
	typedef char8_t utf_cp8_t;
	typedef char16_t utf_cp16_t;
	typedef char32_t utf_cp32_t;
#elif defined(__cpp_unicode_characters)
	typedef unsigned char utf_cp8_t;
	typedef char16_t utf_cp16_t;
	typedef char32_t utf_cp32_t;
#else
	typedef unsigned char utf_cp8_t;
	typedef uint16_t utf_cp16_t;
	typedef uint32_t utf_cp32_t;
#endif

	namespace UtfImpl
	{
		namespace Utils
		{
			template<typename value_type, typename flag_type>
			UTF_CONSTEXPR UTF_NODISCARD
				bool IsFlagSet(value_type Value, flag_type Flag) noexcept
			{
				return (Value & Flag) == Flag;
			}

			template<typename value_type, typename flag_type>
			UTF_CONSTEXPR UTF_NODISCARD
				value_type GetWithClearedFlag(value_type Value, flag_type Flag) noexcept
			{
				return static_cast<value_type>(Value & static_cast<flag_type>(~Flag));
			}

			// Does not add/remove cv-qualifiers
			template<typename target_type, typename current_type>
			UTF_CONSTEXPR UTF_NODISCARD
				auto ForceCastIfMissmatch(current_type&& Arg) -> std::enable_if_t<std::is_same<std::decay_t<target_type>, std::decay_t<current_type>>::value, current_type>
			{
				return static_cast<current_type>(Arg);
			}

			// Does not add/remove cv-qualifiers
			template<typename target_type, typename current_type>
			UTF_CONSTEXPR UTF_NODISCARD
				auto ForceCastIfMissmatch(current_type&& Arg) -> std::enable_if_t<!std::is_same<std::decay_t<target_type>, std::decay_t<current_type>>::value, target_type>
			{
				return reinterpret_cast<target_type>(Arg);
			}
		}

		// wchar_t is a utf16 codepoint on windows, utf32 on linux
		UTF_CONSTEXPR bool IsWCharUtf32 = sizeof(wchar_t) == 0x4;

		// Any value greater than this is not a valid Unicode symbol
		UTF_CONSTEXPR utf_cp32_t MaxValidUnicodeChar = 0x10FFFF;

		namespace Utf8
		{
			/*
			* Available bits, and max values, for n-byte utf8 characters
			*
			* 01111111 -> 1 byte  -> 7 bits
			* 11011111 -> 2 bytes -> 5 + 6 bits -> 11 bits
			* 11101111 -> 3 bytes -> 4 + 6 + 6 bits -> 16 bits
			* 11110111 -> 4 bytes -> 3 + 6 + 6 + 6 bits -> 21 bits
			*
			* 10111111 -> follow up byte
			*/
			UTF_CONSTEXPR utf_cp32_t Max1ByteValue = (1 <<  7) - 1; //  7 bits available
			UTF_CONSTEXPR utf_cp32_t Max2ByteValue = (1 << 11) - 1; // 11 bits available
			UTF_CONSTEXPR utf_cp32_t Max3ByteValue = (1 << 16) - 1; // 16 bits available
			UTF_CONSTEXPR utf_cp32_t Max4ByteValue = 0x10FFFF;      // 21 bits available, but not fully used

			// Flags for follow-up bytes of multibyte utf8 character
			UTF_CONSTEXPR utf_cp8_t FollowupByteMask = 0b1000'0000;
			UTF_CONSTEXPR utf_cp8_t FollowupByteDataMask = 0b0011'1111;
			UTF_CONSTEXPR utf_cp8_t NumDataBitsInFollowupByte = 0x6;

			// Flags for start-bytes of multibyte utf8 characters
			UTF_CONSTEXPR utf_cp8_t TwoByteFlag = 0b1100'0000;
			UTF_CONSTEXPR utf_cp8_t ThreeByteFlag = 0b1110'0000;
			UTF_CONSTEXPR utf_cp8_t FourByteFlag = 0b1111'0000;

			UTF_CONSTEXPR UTF_NODISCARD
				bool IsValidFollowupCodepoint(const utf_cp8_t Codepoint) noexcept
			{
				// Test the upper 2 bytes for the FollowupByteMask
				return (Codepoint & 0b1100'0000) == FollowupByteMask;
			}

			template<int ByteSize>
			UTF_CONSTEXPR UTF_NODISCARD
				bool IsValidUtf8Sequence(const utf_cp8_t FirstCp, const utf_cp8_t SecondCp, const utf_cp8_t ThirdCp, const utf_cp8_t FourthCp) noexcept
			{
				switch (ByteSize)
				{
				case 1:
				{
					return SecondCp == 0 && ThirdCp == 0 && FourthCp == 0;
				}
				case 4:
				{
					const bool bIsOverlongEncoding = Utils::GetWithClearedFlag(FirstCp, ~Utf8::TwoByteFlag) != 0 && SecondCp == Utf8::FollowupByteMask;
					return !bIsOverlongEncoding && IsValidFollowupCodepoint(SecondCp) && IsValidFollowupCodepoint(ThirdCp) && IsValidFollowupCodepoint(FourthCp);
				}
				case 3:
				{
					const bool bIsOverlongEncoding = Utils::GetWithClearedFlag(FirstCp, ~Utf8::ThreeByteFlag) != 0 && SecondCp == Utf8::FollowupByteMask;
					return !bIsOverlongEncoding && IsValidFollowupCodepoint(SecondCp) && IsValidFollowupCodepoint(ThirdCp) && FourthCp == 0;
				}
				case 2:
				{
					const bool bIsOverlongEncoding = Utils::GetWithClearedFlag(FirstCp, ~Utf8::FourByteFlag) != 0 && SecondCp == Utf8::FollowupByteMask;
					return !bIsOverlongEncoding && IsValidFollowupCodepoint(SecondCp) && ThirdCp == 0 && FourthCp == 0;
				}
				default:
				{
					return false;
					break;
				}
				}
			}
		}

		namespace Utf16
		{
			// Surrogate masks and offset for multibyte utf16 characters
			UTF_CONSTEXPR utf_cp16_t HighSurrogateRangeStart = 0xD800;
			UTF_CONSTEXPR utf_cp16_t LowerSurrogateRangeStart = 0xDC00;

			UTF_CONSTEXPR utf_cp32_t SurrogatePairOffset = 0x10000;

			// Unicode range for 2byte utf16 values
			UTF_CONSTEXPR utf_cp32_t SurrogateRangeLowerBounds = 0xD800;
			UTF_CONSTEXPR utf_cp32_t SurrogateRangeUpperBounds = 0xDFFF;


			UTF_CONSTEXPR UTF_NODISCARD
				bool IsHighSurrogate(const utf_cp16_t Codepoint) noexcept
			{
				// Range [0xD800 - 0xDC00[
				return Codepoint >= HighSurrogateRangeStart && Codepoint < LowerSurrogateRangeStart;
			}
			UTF_CONSTEXPR UTF_NODISCARD
				bool IsLowSurrogate(const utf_cp16_t Codepoint) noexcept
			{
				// Range [0xDC00 - 0xDFFF]
				return Codepoint >= LowerSurrogateRangeStart && Codepoint <= SurrogateRangeUpperBounds;
			}

			// Tests if a utf16 value is a valid Unicode character
			UTF_CONSTEXPR UTF_NODISCARD
				bool IsValidUnicodeChar(const utf_cp16_t LowerCodepoint, const utf_cp16_t UpperCodepoint) noexcept
			{
				const bool IsValidHighSurrogate = IsHighSurrogate(UpperCodepoint);
				const bool IsValidLowSurrogate = IsLowSurrogate(LowerCodepoint);

				// Both needt to be valid
				if (IsValidHighSurrogate)
					return IsValidLowSurrogate;

				// Neither are valid && the codepoints are not in the wrong surrogate ranges
				return !IsValidLowSurrogate && !IsHighSurrogate(LowerCodepoint) && !IsLowSurrogate(UpperCodepoint);
			}
		}

		namespace Utf32
		{
			// Tests if a utf32 value is a valid Unicode character
			UTF_CONSTEXPR UTF_NODISCARD
				bool IsValidUnicodeChar(const utf_cp32_t Codepoint) noexcept
			{
				// Codepoints must be within the valid unicode range and must not be within the range of Surrogate-values
				return Codepoint < MaxValidUnicodeChar && (Codepoint < Utf16::SurrogateRangeLowerBounds || Codepoint > Utf16::SurrogateRangeUpperBounds);
			}
		}

		namespace Iterator
		{
			template<typename child_type>
			class utf_char_iterator_base_child_acessor
			{
			private:
				template<class child_iterator_type, typename codepoint_iterator_type, typename utf_char_type>
				friend class utf_char_iterator_base;

			private:
				static UTF_CONSTEXPR
					void ReadChar(child_type* This)
				{
					return This->ReadChar();
				}
			};

			template<class child_iterator_type, typename codepoint_iterator_type, typename utf_char_type>
			class utf_char_iterator_base
			{
			public:
				UTF_CONSTEXPR utf_char_iterator_base(codepoint_iterator_type Begin, codepoint_iterator_type End)
					: CurrentIterator(Begin), NextCharStartIterator(Begin), EndIterator(End)
				{
					utf_char_iterator_base_child_acessor<child_iterator_type>::ReadChar(static_cast<child_iterator_type*>(this));
				}

				template<typename container_type,
					typename = decltype(std::begin(std::declval<container_type>())), // Has begin
					typename = decltype(std::end(std::declval<container_type>())),   // Has end
					typename iterator_deref_type = decltype(*std::end(std::declval<container_type>())), // Iterator can be dereferenced
					typename = std::enable_if<sizeof(std::decay<iterator_deref_type>::type) == utf_char_type::GetCodepointSize()>::type // Return-value of derferenced iterator has the same size as one codepoint
				>
				explicit UTF_CONSTEXPR utf_char_iterator_base(container_type& Container)
					: CurrentIterator(std::begin(Container)), NextCharStartIterator(std::begin(Container)), EndIterator(std::end(Container))
				{
					utf_char_iterator_base_child_acessor<child_iterator_type>::ReadChar(static_cast<child_iterator_type*>(this));
				}

			public:
				UTF_CONSTEXPR inline
					child_iterator_type& operator++()
				{
					// Skip ahead to the next char
					CurrentIterator = NextCharStartIterator;

					// Populate the current char and advance the NextCharStartIterator
					utf_char_iterator_base_child_acessor<child_iterator_type>::ReadChar(static_cast<child_iterator_type*>(this));


					return *static_cast<child_iterator_type*>(this);
				}

			public:
				UTF_CONSTEXPR inline
					utf_char_type operator*() const
				{
					return CurrentChar;
				}

				UTF_CONSTEXPR inline bool operator==(const child_iterator_type& Other) const
				{
					return CurrentIterator == Other.CurrentIterator;
				}
				UTF_CONSTEXPR inline
					bool operator!=(const child_iterator_type& Other) const
				{
					return CurrentIterator != Other.CurrentIterator;
				}

				UTF_CONSTEXPR inline
					explicit operator bool() const
				{
					return this->CurrentIterator != this->EndIterator;
				}

			public:
				UTF_CONSTEXPR inline 
					child_iterator_type begin()
				{
					return *static_cast<child_iterator_type*>(this);
				}

				UTF_CONSTEXPR inline
					child_iterator_type end()
				{
					return child_iterator_type(EndIterator, EndIterator);
				}

			protected:
				codepoint_iterator_type CurrentIterator; // Current byte pos
				codepoint_iterator_type NextCharStartIterator; // Byte pos of the next character
				codepoint_iterator_type EndIterator; // End Iterator

				utf_char_type CurrentChar; // Current character bytes
			};
		}
	}

	struct utf8_bytes
	{
		utf_cp8_t Codepoints[4] = { 0 };
	};

	struct utf16_pair
	{
		utf_cp16_t Lower = 0;
		utf_cp16_t Upper = 0;
	};

	UTF_CONSTEXPR inline
		bool operator==(const utf8_bytes Left, const utf8_bytes Right) noexcept
	{
		return Left.Codepoints[0] == Right.Codepoints[0]
			&& Left.Codepoints[1] == Right.Codepoints[1]
			&& Left.Codepoints[2] == Right.Codepoints[2]
			&& Left.Codepoints[3] == Right.Codepoints[3];
	}
	UTF_CONSTEXPR inline
		bool operator!=(const utf8_bytes Left, const utf8_bytes Right) noexcept
	{
		return !(Left == Right);
	}

	UTF_CONSTEXPR inline
		bool operator==(const utf16_pair Left, const utf16_pair Right) noexcept
	{
		return Left.Upper == Right.Upper && Left.Lower == Right.Lower;
	}
	UTF_CONSTEXPR inline
		bool operator!=(const utf16_pair Left, const utf16_pair Right) noexcept
	{
		return !(Left == Right);
	}


	enum class UtfEncodingType
	{
		Invalid,
		Utf8,
		Utf16,
		Utf32
	};

	template<UtfEncodingType Encoding>
	struct utf_char;

	typedef utf_char<UtfEncodingType::Utf8> utf_char8;
	typedef utf_char<UtfEncodingType::Utf16> utf_char16;
	typedef utf_char<UtfEncodingType::Utf32> utf_char32;

	template<>
	struct utf_char<UtfEncodingType::Utf8>
	{
		utf8_bytes Char = { 0 };

	public:
		UTF_CONSTEXPR utf_char() = default;
		UTF_CONSTEXPR utf_char(utf_char&&) = default;
		UTF_CONSTEXPR utf_char(const utf_char&) = default;

		UTF_CONSTEXPR utf_char(utf8_bytes InChar) noexcept;

		template<typename char_type, typename = decltype(ParseUtf8CharFromStr(std::declval<const char_type*>()))>
		UTF_CONSTEXPR utf_char(const char_type* SingleCharString) noexcept;

	public:
		UTF_CONSTEXPR utf_char& operator=(utf_char&&) = default;
		UTF_CONSTEXPR utf_char& operator=(const utf_char&) = default;

		UTF_CONSTEXPR utf_char& operator=(utf8_bytes inBytse) noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD       utf_cp8_t&  operator[](const uint8_t Index);
		UTF_CONSTEXPR UTF_NODISCARD const utf_cp8_t&  operator[](const uint8_t Index) const;

		UTF_CONSTEXPR UTF_NODISCARD bool operator==(utf_char8 Other) const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD bool operator!=(utf_char8 Other) const noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD utf_char8 GetAsUtf8() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char16 GetAsUtf16() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char32 GetAsUtf32() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD utf8_bytes Get() const;

		UTF_CONSTEXPR UTF_NODISCARD UtfEncodingType GetEncoding() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD uint8_t GetNumCodepoints() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD static uint8_t GetCodepointSize() noexcept;
	};

	template<>
	struct utf_char<UtfEncodingType::Utf16>
	{
		utf16_pair Char = { 0 };

	public:
		UTF_CONSTEXPR utf_char() = default;
		UTF_CONSTEXPR utf_char(utf_char&&) = default;
		UTF_CONSTEXPR utf_char(const utf_char&) = default;

		UTF_CONSTEXPR utf_char(utf16_pair InChar) noexcept;

		template<typename char_type, typename = decltype(ParseUtf16CharFromStr(std::declval<const char_type*>()))>
		UTF_CONSTEXPR utf_char(const char_type* SingleCharString) noexcept;

	public:
		UTF_CONSTEXPR utf_char& operator=(utf_char&&) = default;
		UTF_CONSTEXPR utf_char& operator=(const utf_char&) = default;

		UTF_CONSTEXPR utf_char& operator=(utf16_pair inBytse) noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD bool operator==(utf_char16 Other) const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD bool operator!=(utf_char16 Other) const noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD utf_char8 GetAsUtf8() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char16 GetAsUtf16() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char32 GetAsUtf32() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD utf16_pair Get() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD UtfEncodingType GetEncoding() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD uint8_t GetNumCodepoints() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD static uint8_t GetCodepointSize() noexcept;
	};
)";

	UnicodeLib << R"(
	template<>
	struct utf_char<UtfEncodingType::Utf32>
	{
		utf_cp32_t Char = { 0 };

	public:
		UTF_CONSTEXPR utf_char() = default;
		UTF_CONSTEXPR utf_char(utf_char&&) = default;
		UTF_CONSTEXPR utf_char(const utf_char&) = default;

		UTF_CONSTEXPR utf_char(utf_cp32_t InChar) noexcept;

		template<typename char_type, typename = decltype(ParseUtf32CharFromStr(std::declval<const char_type*>()))>
		UTF_CONSTEXPR utf_char(const char_type* SingleCharString) noexcept;

	public:
		UTF_CONSTEXPR utf_char& operator=(utf_char&&) = default;
		UTF_CONSTEXPR utf_char& operator=(const utf_char&) = default;

		UTF_CONSTEXPR utf_char& operator=(utf_cp32_t inBytse) noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD bool operator==(utf_char32 Other) const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD bool operator!=(utf_char32 Other) const noexcept;

	public:
		UTF_CONSTEXPR UTF_NODISCARD utf_char8 GetAsUtf8() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char16 GetAsUtf16() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD utf_char32 GetAsUtf32() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD utf_cp32_t Get() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD UtfEncodingType GetEncoding() const noexcept;
		UTF_CONSTEXPR UTF_NODISCARD uint8_t GetNumCodepoints() const noexcept;

		UTF_CONSTEXPR UTF_NODISCARD static uint8_t GetCodepointSize() noexcept;
	};

	UTF_CONSTEXPR UTF_NODISCARD
		uint8_t GetUtf8CharLenght(const utf_cp8_t C) noexcept
	{
		using namespace UtfImpl;

		/* No flag for any other byte-count is set */
		if ((C & 0b1000'0000) == 0)
		{
			return 0x1;
		}
		else if (Utils::IsFlagSet(C, Utf8::FourByteFlag))
		{
			return 0x4;
		}
		else if (Utils::IsFlagSet(C, Utf8::ThreeByteFlag))
		{
			return 0x3;
		}
		else if (Utils::IsFlagSet(C, Utf8::TwoByteFlag))
		{
			return 0x2;
		}
		else
		{
			/* Invalid! This is a follow up codepoint but conversion needs to start at the start-codepoint. */
			return 0x0;
		}
	}

	UTF_CONSTEXPR UTF_NODISCARD
		uint8_t GetUtf16CharLenght(const utf_cp16_t UpperCodepoint) noexcept
	{
		if (UtfImpl::Utf16::IsHighSurrogate(UpperCodepoint))
			return 0x2;

		return 0x1;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char16 Utf32ToUtf16Pair(const utf_char32 Character) noexcept
	{
		using namespace UtfImpl;

		if (!Utf32::IsValidUnicodeChar(Character.Char))
			return utf16_pair{};

		utf16_pair RetCharPair;

		if (Character.Char > USHRT_MAX)
		{
			const utf_cp32_t PreparedCodepoint = Character.Char - Utf16::SurrogatePairOffset;

			RetCharPair.Upper = (PreparedCodepoint >> 10) & 0b1111111111;
			RetCharPair.Lower = PreparedCodepoint & 0b1111111111;

			// Surrogate-pair starting ranges for higher/lower surrogates
			RetCharPair.Upper += Utf16::HighSurrogateRangeStart;
			RetCharPair.Lower += Utf16::LowerSurrogateRangeStart;

			return RetCharPair;
		}

		RetCharPair.Lower = static_cast<utf_cp16_t>(Character.Char);

		return RetCharPair;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char32 Utf16PairToUtf32(const utf_char16 Character) noexcept
	{
		using namespace UtfImpl;

		// The surrogate-values are not valid Unicode codepoints
		if (!Utf16::IsValidUnicodeChar(Character.Char.Lower, Character.Char.Upper))
			return utf_cp32_t{ 0 };

		if (Character.Char.Upper)
		{
			// Move the characters back from the surrogate range to the normal range
			const utf_cp16_t UpperCodepointWithoutSurrogate = static_cast<utf_cp16_t>(Character.Char.Upper - Utf16::HighSurrogateRangeStart);
			const utf_cp16_t LowerCodepointWithoutSurrogate = static_cast<utf_cp16_t>(Character.Char.Lower - Utf16::LowerSurrogateRangeStart);

			return ((static_cast<utf_cp32_t>(UpperCodepointWithoutSurrogate) << 10) | LowerCodepointWithoutSurrogate) + Utf16::SurrogatePairOffset;
		}

		return Character.Char.Lower;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char8 Utf32ToUtf8Bytes(const utf_char32 Character) noexcept
	{
		using namespace UtfImpl;
		using namespace UtfImpl::Utf8;

		if (!Utf32::IsValidUnicodeChar(Character.Char))
			return utf_char8{};

		utf8_bytes RetBytes;

		if (Character.Char <= Max1ByteValue)
		{
			RetBytes.Codepoints[0] = static_cast<utf_cp8_t>(Character.Char);
		}
		else if (Character.Char <= Max2ByteValue)
		{
			/* Upper 3 bits of first byte are reserved for byte-lengh. */
			RetBytes.Codepoints[0] = TwoByteFlag;
			RetBytes.Codepoints[0] |= Character.Char >> NumDataBitsInFollowupByte; // Lower bits stored in 2nd byte

			RetBytes.Codepoints[1] |= FollowupByteMask;
			RetBytes.Codepoints[1] |= Character.Char & FollowupByteDataMask;
		}
		else if (Character.Char <= Max3ByteValue)
		{
			/* Upper 4 bits of first byte are reserved for byte-lengh. */
			RetBytes.Codepoints[0] = ThreeByteFlag;
			RetBytes.Codepoints[0] |= Character.Char >> (NumDataBitsInFollowupByte * 2); // Lower bits stored in 2nd and 3rd bytes

			RetBytes.Codepoints[1] = FollowupByteMask;
			RetBytes.Codepoints[1] |= (Character.Char >> NumDataBitsInFollowupByte) & FollowupByteDataMask; // Lower bits stored in 2nd byte

			RetBytes.Codepoints[2] = FollowupByteMask;
			RetBytes.Codepoints[2] |= Character.Char & FollowupByteDataMask;
		}
		else if (Character.Char <= Max4ByteValue)
		{
			/* Upper 5 bits of first byte are reserved for byte-lengh. */
			RetBytes.Codepoints[0] = FourByteFlag;
			RetBytes.Codepoints[0] |= Character.Char >> (NumDataBitsInFollowupByte * 3); // Lower bits stored in 2nd, 3rd and 4th bytes

			RetBytes.Codepoints[1] = FollowupByteMask;
			RetBytes.Codepoints[1] |= (Character.Char >> (NumDataBitsInFollowupByte * 2)) & FollowupByteDataMask; // Lower bits stored in 3rd and 4th bytes

			RetBytes.Codepoints[2] = FollowupByteMask;
			RetBytes.Codepoints[2] |= (Character.Char >> NumDataBitsInFollowupByte) & FollowupByteDataMask; // Lower bits stored in 4th byte

			RetBytes.Codepoints[3] = FollowupByteMask;
			RetBytes.Codepoints[3] |= Character.Char & FollowupByteDataMask;
		}
		else
		{
			/* Above max allowed value. Invalid codepoint. */
			return RetBytes;
		}

		return RetBytes;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_cp32_t Utf8BytesToUtf32(const utf_char8 Character) noexcept
	{
		using namespace UtfImpl;
		using namespace UtfImpl::Utf8;

		/* No flag for any other byte-count is set */
		if ((Character[0] & 0b1000'0000) == 0)
		{
			if (!Utf8::IsValidUtf8Sequence<1>(Character[0], Character[1], Character[2], Character[3])) // Verifies encoding
				return utf_cp32_t{ 0 };

			return Character[0];
		}
		else if (Utils::IsFlagSet(Character[0], FourByteFlag))
		{
			utf_cp32_t RetChar = Utils::GetWithClearedFlag(Character[3], FollowupByteMask);
			RetChar |= Utils::GetWithClearedFlag(Character[2], FollowupByteMask) << (NumDataBitsInFollowupByte * 1); // Clear the FollowupByteMask and move the bits to the right position
			RetChar |= Utils::GetWithClearedFlag(Character[1], FollowupByteMask) << (NumDataBitsInFollowupByte * 2); // Clear the FollowupByteMask and move the bits to the right position
			RetChar |= Utils::GetWithClearedFlag(Character[0], FourByteFlag) << (NumDataBitsInFollowupByte * 3); // Clear the FourByteFlag and move the bits to the right position

			if (!Utf8::IsValidUtf8Sequence<4>(Character[0], Character[1], Character[2], Character[3])  // Verifies encoding
				|| !Utf32::IsValidUnicodeChar(RetChar)) // Verifies ranges
				return utf_cp32_t{ 0 };

			return RetChar;
		}
		else if (Utils::IsFlagSet(Character[0], ThreeByteFlag))
		{
			utf_cp32_t RetChar = Utils::GetWithClearedFlag(Character[2], FollowupByteMask);
			RetChar |= Utils::GetWithClearedFlag(Character[1], FollowupByteMask) << (NumDataBitsInFollowupByte * 1); // Clear the FollowupByteMask and move the bits to the right position
			RetChar |= Utils::GetWithClearedFlag(Character[0], ThreeByteFlag) << (NumDataBitsInFollowupByte * 2); // Clear the ThreeByteFlag and move the bits to the right position

			if (!Utf8::IsValidUtf8Sequence<3>(Character[0], Character[1], Character[2], Character[3]) // Verifies encoding
				|| !Utf32::IsValidUnicodeChar(RetChar)) // Verifies ranges
				return utf_cp32_t{ 0 };

			return RetChar;
		}
		else if (Utils::IsFlagSet(Character[0], TwoByteFlag))
		{
			utf_cp32_t RetChar = Utils::GetWithClearedFlag(Character[1], FollowupByteMask); // Clear the FollowupByteMask and move the bits to the right position
			RetChar |= Utils::GetWithClearedFlag(Character[0], TwoByteFlag) << NumDataBitsInFollowupByte; // Clear the TwoByteFlag and move the bits to the right position

			if (!Utf8::IsValidUtf8Sequence<2>(Character[0], Character[1], Character[2], Character[3]) // Verifies encoding
				|| !Utf32::IsValidUnicodeChar(RetChar)) // Verifies ranges
				return utf_cp32_t{ 0 };

			return RetChar;
		}
		else
		{
			/* Invalid! This is a follow up codepoint but conversion needs to start at the start-codepoint. */
			return 0;
		}
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char8 Utf16PairToUtf8Bytes(const utf_char16 Character) noexcept
	{
		return Utf32ToUtf8Bytes(Utf16PairToUtf32(Character));
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char16 Utf8BytesToUtf16(const utf_char8 Character) noexcept
	{
		return Utf32ToUtf16Pair(Utf8BytesToUtf32(Character));
	}

	template<
		typename codepoint_iterator_type,
		typename iterator_deref_type = decltype(*std::declval<codepoint_iterator_type>()), // Iterator can be dereferenced
		typename = typename std::enable_if<sizeof(std::decay<iterator_deref_type>::type) == utf_char8::GetCodepointSize()>::type // Return-value of derferenced iterator has the same size as one codepoint
	>
	class utf8_iterator : public UtfImpl::Iterator::utf_char_iterator_base<utf8_iterator<codepoint_iterator_type>, codepoint_iterator_type, utf_char8>
	{
	private:
		typedef typename utf8_iterator<codepoint_iterator_type> own_type;

		friend UtfImpl::Iterator::utf_char_iterator_base_child_acessor<own_type>;

	public:
		using UtfImpl::Iterator::utf_char_iterator_base<own_type, codepoint_iterator_type, utf_char8>::utf_char_iterator_base;

	public:
		utf8_iterator() = delete;

	private:
		void ReadChar()
		{
			if (this->NextCharStartIterator == this->EndIterator)
				return;

			// Reset the bytes of the character
			this->CurrentChar = utf8_bytes{ 0 };

			const int CharCodepointCount = GetUtf8CharLenght(static_cast<utf_cp8_t>(*this->NextCharStartIterator));

			for (int i = 0; i < CharCodepointCount; i++)
			{
				// The least character ended abruptly
				if (this->NextCharStartIterator == this->EndIterator)
				{
					this->CurrentIterator = this->EndIterator;
					this->CurrentChar = utf8_bytes{ 0 };
					break;
				}

				this->CurrentChar[static_cast<uint8_t>(i)] = static_cast<utf_cp8_t>(*this->NextCharStartIterator);
				this->NextCharStartIterator++;
			}
		}
	};

	template<
		typename codepoint_iterator_type,
		typename iterator_deref_type = decltype(*std::declval<codepoint_iterator_type>()), // Iterator can be dereferenced
		typename = typename std::enable_if<sizeof(std::decay<iterator_deref_type>::type) == utf_char16::GetCodepointSize()>::type // Return-value of derferenced iterator has the same size as one codepoint
	>
	class utf16_iterator : public UtfImpl::Iterator::utf_char_iterator_base<utf16_iterator<codepoint_iterator_type>, codepoint_iterator_type, utf_char16>
	{
	private:
		typedef typename utf16_iterator<codepoint_iterator_type> own_type;

		friend UtfImpl::Iterator::utf_char_iterator_base_child_acessor<own_type>;

	public:
		using UtfImpl::Iterator::utf_char_iterator_base<own_type, codepoint_iterator_type, utf_char16>::utf_char_iterator_base;

	public:
		utf16_iterator() = delete;

	private:
		UTF_CONSTEXPR void ReadChar()
		{
			if (this->NextCharStartIterator == this->EndIterator)
				return;

			// Reset the bytes of the character
			this->CurrentChar = utf16_pair{ 0 };

			const int CharCodepointCount = GetUtf16CharLenght(static_cast<utf_cp16_t>(*this->NextCharStartIterator));
			
			if (CharCodepointCount == 0x1)
			{
				// Read the only codepoint
				this->CurrentChar.Char.Lower = *this->NextCharStartIterator;
				this->NextCharStartIterator++;

				return;
			}

			// Read the first of two codepoints
			this->CurrentChar.Char.Upper = *this->NextCharStartIterator;
			this->NextCharStartIterator++;

			// The least character ended abruptly
			if (this->NextCharStartIterator == this->EndIterator)
			{
				this->CurrentChar = utf16_pair{ 0 };
				this->CurrentIterator = this->EndIterator;
				return;
			}

			// Read the second of two codepoints
			this->CurrentChar.Char.Lower = *this->NextCharStartIterator;
			this->NextCharStartIterator++;
		}
	};

	template<
		typename codepoint_iterator_type,
		typename iterator_deref_type = decltype(*std::declval<codepoint_iterator_type>()), // Iterator can be dereferenced
		typename = typename std::enable_if<sizeof(std::decay<iterator_deref_type>::type) == utf_char32::GetCodepointSize()>::type // Return-value of derferenced iterator has the same size as one codepoint
	>
	class utf32_iterator : public UtfImpl::Iterator::utf_char_iterator_base<utf32_iterator<codepoint_iterator_type>, codepoint_iterator_type, utf_char32>
	{
	private:
		typedef typename utf32_iterator<codepoint_iterator_type> own_type;

		friend UtfImpl::Iterator::utf_char_iterator_base_child_acessor<own_type>;

	public:
		using UtfImpl::Iterator::utf_char_iterator_base<own_type, codepoint_iterator_type, utf_char32>::utf_char_iterator_base;

	public:
		utf32_iterator() = delete;

	public:
		template<typename char_type = utf_cp32_t>
		auto Replace(const char_type NewChar) -> std::enable_if_t<std::is_assignable<iterator_deref_type, char_type>::value>
		{
			this->CurrentChar = NewChar;
			*this->CurrentIterator = NewChar;
		}

	private:
		void ReadChar()
		{
			if (this->NextCharStartIterator == this->EndIterator)
				return;

			this->CurrentChar = *this->NextCharStartIterator;
			this->NextCharStartIterator++;
		}
	};

	template<typename codepoint_type,
		typename std::enable_if<sizeof(codepoint_type) == 0x1 && std::is_integral<codepoint_type>::value, int>::type = 0
	>
	UTF_CONSTEXPR UTF_NODISCARD
		utf_char8 ParseUtf8CharFromStr(const codepoint_type* Str)
	{
		if (!Str)
			return utf8_bytes{};

		const utf_cp8_t FirstCodepoint = static_cast<utf_cp8_t>(Str[0]);
		const auto CharLength = GetUtf8CharLenght(FirstCodepoint);

		if (CharLength == 0)
			return utf8_bytes{};

		utf8_bytes RetChar;
		RetChar.Codepoints[0] = FirstCodepoint;

		for (int i = 1; i < CharLength; i++)
		{
			const utf_cp8_t CurrentCodepoint = static_cast<utf_cp8_t>(Str[i]);

			// Filters the null-terminator and other invalid followup bytes
			if (!UtfImpl::Utf8::IsValidFollowupCodepoint(CurrentCodepoint))
				return utf8_bytes{};

			RetChar.Codepoints[i] = CurrentCodepoint;
		}

		return RetChar;
	}

	template<typename codepoint_type,
		typename std::enable_if<sizeof(codepoint_type) == 0x2 && std::is_integral<codepoint_type>::value, int>::type = 0
	>
	UTF_CONSTEXPR UTF_NODISCARD
		utf_char16 ParseUtf16CharFromStr(const codepoint_type* Str)
	{
		if (!Str)
			return utf_char16{};

		const utf_cp16_t FirstCodepoint = static_cast<utf_cp16_t>(Str[0]);

		if (GetUtf16CharLenght(FirstCodepoint) == 1)
			return utf16_pair{ FirstCodepoint };

		return utf16_pair{ FirstCodepoint,  static_cast<utf_cp16_t>(Str[1]) };
	}

	template<typename codepoint_type,
		typename std::enable_if<sizeof(codepoint_type) == 0x4 && std::is_integral<codepoint_type>::value, int>::type = 0
	>
	UTF_CONSTEXPR UTF_NODISCARD
		utf_char32 ParseUtf32CharFromStr(const codepoint_type* Str)
	{
		if (!Str)
			return utf_char32{};

		return static_cast<utf_cp32_t>(Str[0]);
	}

)";

	UnicodeLib << R"(
	/*
	 * Conversions from UTF-16 to UTF-8
	 */
	template<typename utf8_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf8_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf16StringToUtf8String(utf16_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf8_char_string RetString;

		for (const utf_char16 Char : StringIteratorToConvert)
		{
			const auto NewChar = Utf16PairToUtf8Bytes(Char);

			for (int i = 0; i < NewChar.GetNumCodepoints(); i++)
				RetString += static_cast<target_char_type>(NewChar[static_cast<uint8_t>(i)]);
		}

		return RetString;
	}

	template<typename utf8_char_string, typename utf16_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<const utf16_char_string>())),
		typename = utf16_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf16StringToUtf8String(const utf16_char_string& StringToConvert)
	{
		return Utf16StringToUtf8String<utf8_char_string>(utf16_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf8_char_string, typename utf16_char_type, size_t CStrLenght,
		typename = utf16_iterator<utf16_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf16StringToUtf8String(utf16_char_type(&StringToConvert)[CStrLenght])
	{
		return Utf16StringToUtf8String<utf8_char_string>(utf16_iterator<const utf16_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf8_char_string, typename utf16_char_type,
		typename = utf16_iterator<utf16_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf16StringToUtf8String(const utf16_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf16StringToUtf8String<utf8_char_string>(utf16_iterator<const utf16_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	/*
	 * Conversions from UTF-32 to UTF-8
	 */
	template<typename utf8_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf8_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf32StringToUtf8String(utf32_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf8_char_string RetString;

		for (const utf_char32 Char : StringIteratorToConvert)
		{
			const auto NewChar = Utf32ToUtf8Bytes(Char);

			for (int i = 0; i < NewChar.GetNumCodepoints(); i++)
				RetString += static_cast<target_char_type>(NewChar[static_cast<uint8_t>(i)]);
		}

		return RetString;
	}

	template<typename utf8_char_string, typename utf32_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<utf32_char_string>())),
		typename = utf32_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf32StringToUtf8String(const utf32_char_string& StringToConvert)
	{
		return Utf32StringToUtf8String<utf8_char_string>(utf32_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf8_char_string, typename utf32_char_type, size_t cstr_lenght,
		typename = utf32_iterator<utf32_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf32StringToUtf8String(utf32_char_type(&StringToConvert)[cstr_lenght])
	{
		return Utf32StringToUtf8String<utf8_char_string>(utf32_iterator<const utf32_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf8_char_string, typename utf32_char_type,
		typename = utf32_iterator<utf32_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf8_char_string Utf32StringToUtf8String(const utf32_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf32StringToUtf8String<utf8_char_string>(utf32_iterator<const utf32_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	/*
	 * Conversions from UTF-8 to UTF-16
	 */
	template<typename utf16_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf16_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf8StringToUtf16String(utf8_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf16_char_string RetString;

		for (const utf_char8 Char : StringIteratorToConvert)
		{
			const auto NewChar = Utf8BytesToUtf16(Char);

			if (NewChar.GetNumCodepoints() > 1)
				RetString += NewChar.Get().Upper;

			RetString += NewChar.Get().Lower;
		}

		return RetString;
	}

	template<typename utf16_char_string, typename utf8_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<utf8_char_string>())),
		typename = utf8_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf8StringToUtf16String(const utf8_char_string& StringToConvert)
	{
		return Utf8StringToUtf16String<utf16_char_string>(utf8_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf16_char_string, typename utf8_char_type, size_t cstr_lenght,
		typename = utf8_iterator<utf8_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf8StringToUtf16String(utf8_char_type(&StringToConvert)[cstr_lenght])
	{
		return Utf32StringToUtf16String<utf16_char_string>(utf8_iterator<const utf8_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf16_char_string, typename utf8_char_type,
		typename = utf8_iterator<utf8_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf8StringToUtf16String(const utf8_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf32StringToUtf16String<utf16_char_string>(utf32_iterator<const utf8_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	/*
	 * Conversions from UTF-32 to UTF-16
	 */
	template<typename utf16_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf16_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf32StringToUtf16String(utf32_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf16_char_string RetString;

		for (const utf_char32 Char : StringIteratorToConvert)
		{
			const auto NewChar = Utf32ToUtf16Pair(Char);

			if (NewChar.GetNumCodepoints() > 1)
				RetString += NewChar.Get().Upper;

			RetString += NewChar.Get().Lower;
		}

		return RetString;
	}

	template<typename utf16_char_string, typename utf32_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<utf32_char_string>())),
		typename = utf32_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf32StringToUtf16String(const utf32_char_string& StringToConvert)
	{
		return Utf32StringToUtf16String<utf16_char_string>(utf32_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf16_char_string, typename utf32_char_type, size_t cstr_lenght,
		typename = utf32_iterator<utf32_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf32StringToUtf16String(utf32_char_type(&StringToConvert)[cstr_lenght])
	{
		return Utf32StringToUtf16String<utf16_char_string>(utf32_iterator<const utf32_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf16_char_string, typename utf32_char_type,
		typename = utf8_iterator<utf32_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf16_char_string Utf32StringToUtf16String(const utf32_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf32StringToUtf16String<utf16_char_string>(utf32_iterator<const utf32_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	/*
	 * Conversions from UTF-8 to UTF-32
	 */
	template<typename utf32_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf32_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf8StringToUtf32String(utf8_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf32_char_string RetString;

		for (const utf_char8 Char : StringIteratorToConvert)
		{
			RetString += Utf8BytesToUtf32(Char);
		}

		return RetString;
	}

	template<typename utf32_char_string, typename utf8_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<utf8_char_string>())),
		typename = utf8_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf8StringToUtf32String(const utf8_char_string& StringToConvert)
	{
		return Utf8StringToUtf32String<utf32_char_string>(utf8_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf32_char_string, typename utf8_char_type, size_t cstr_lenght,
		typename = utf8_iterator<utf8_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf8StringToUtf32String(utf8_char_type(&StringToConvert)[cstr_lenght])
	{
		return Utf8StringToUtf32String<utf32_char_string>(utf8_iterator<const utf8_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf32_char_string, typename utf8_char_type, size_t cstr_lenght,
		typename = utf8_iterator<utf8_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf8StringToUtf32String(const utf8_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf8StringToUtf32String<utf32_char_string>(utf8_iterator<const utf8_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	/*
	 * Conversions from UTF-16 to UTF-32
	 */
	template<typename utf32_char_string,
		typename inner_iterator,
		typename target_char_type = typename std::decay<decltype(*std::begin(std::declval<utf32_char_string>()))>::type
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf16StringToUtf32String(utf16_iterator<inner_iterator> StringIteratorToConvert)
	{
		utf32_char_string RetString;

		for (const utf_char16 Char : StringIteratorToConvert)
		{
			RetString += Utf16PairToUtf32(Char).Get();
		}

		return RetString;
	}

	template<typename utf32_char_string, typename utf16_char_string,
		typename inner_iterator = decltype(std::begin(std::declval<const utf16_char_string>())),
		typename = utf16_iterator<inner_iterator>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf16StringToUtf32String(const utf16_char_string& StringToConvert)
	{
		return Utf16StringToUtf32String<utf32_char_string>(utf16_iterator<inner_iterator>(StringToConvert));
	}

	template<typename utf32_char_string, typename utf16_char_type, size_t cstr_lenght,
		typename = utf16_iterator<utf16_char_type*>
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf16StringToUtf32String(utf16_char_type(&StringToConvert)[cstr_lenght])
	{
		return Utf16StringToUtf32String<utf32_char_string>(utf16_iterator<const utf16_char_type*>(std::begin(StringToConvert), std::end(StringToConvert)));
	}

	template<typename utf32_char_string, typename utf16_char_type, size_t cstr_lenght,
		typename = utf16_iterator<utf16_char_type*>
	>
		UTF_CONSTEXPR20 UTF_NODISCARD
		utf32_char_string Utf16StringToUtf32String(const utf16_char_type* StringToConvert, int NonNullTermiantedLength)
	{
		return Utf16StringToUtf32String<utf32_char_string>(utf16_iterator<const utf16_char_type*>(StringToConvert, StringToConvert + NonNullTermiantedLength));
	}


	template<typename wstring_type = std::wstring, typename string_type = std::string,
		typename = decltype(std::begin(std::declval<wstring_type>())), // has 'begin()'
		typename = decltype(std::end(std::declval<wstring_type>()))    // has 'end()'
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		string_type WStringToString(const wstring_type& WideString)
	{
		using char_type = typename std::decay<decltype(*std::begin(std::declval<wstring_type>()))>::type;

		// Workaround to missing 'if constexpr (...)' in Cpp14. Satisfies the requirements of conversion-functions. Safe because the incorrect function is never going to be invoked.
		struct dummy_2byte_str { uint16_t* begin() const { return nullptr; };   uint16_t* end() const { return nullptr; }; };
		struct dummy_4byte_str { uint32_t* begin() const { return nullptr; };   uint32_t* end() const { return nullptr; }; };

		if UTF_IF_CONSTEXPR (sizeof(char_type) == 0x2) // UTF-16
		{
			using type_to_use = typename std::conditional<sizeof(char_type) == 2, wstring_type, dummy_2byte_str>::type;
			return Utf16StringToUtf8String<string_type, type_to_use>(UtfImpl::Utils::ForceCastIfMissmatch<const type_to_use&, const wstring_type&>(WideString));
		}
		else // UTF-32
		{
			using type_to_use = typename std::conditional<sizeof(char_type) == 4, wstring_type, dummy_4byte_str>::type;
			return Utf32StringToUtf8String<string_type, type_to_use>(UtfImpl::Utils::ForceCastIfMissmatch<const type_to_use&, const wstring_type&>(WideString));
		}
	}

	template<typename string_type = std::string, typename wstring_type = std::wstring,
		typename = decltype(std::begin(std::declval<string_type>())), // has 'begin()'
		typename = decltype(std::end(std::declval<string_type>()))    // has 'end()'
	>
	UTF_CONSTEXPR20 UTF_NODISCARD
		wstring_type StringToWString(const string_type& NarrowString)
	{
		using char_type = typename std::decay<decltype(*std::begin(std::declval<wstring_type>()))>::type;

		// Workaround to missing 'if constexpr (...)' in Cpp14. Satisfies the requirements of conversion-functions. Safe because the incorrect function is never going to be invoked.
		struct dummy_2byte_str { uint16_t* begin() const { return nullptr; };   uint16_t* end() const { return nullptr; }; };
		struct dummy_4byte_str { uint32_t* begin() const { return nullptr; };   uint32_t* end() const { return nullptr; }; };

		if UTF_IF_CONSTEXPR(sizeof(char_type) == 0x2) // UTF-16
		{
			using type_to_use = typename std::conditional<sizeof(char_type) == 2, wstring_type, dummy_2byte_str>::type;
			return Utf8StringToUtf16String<type_to_use, string_type>(NarrowString);
		}
		else // UTF-32
		{
			using type_to_use = typename std::conditional<sizeof(char_type) == 4, wstring_type, dummy_4byte_str>::type;
			return Utf8StringToUtf32String<type_to_use, string_type>(NarrowString);
		}
	}

)";

	UnicodeLib << R"(
	template<typename byte_iterator_type>
	UTF_CONSTEXPR byte_iterator_type ReplaceUtf8(byte_iterator_type Begin, byte_iterator_type End, utf_cp8_t CharToReplace, utf_cp8_t ReplacementChar)
	{
		using namespace UtfImpl;

		if (Begin == End)
			return End;

		const auto ToReplaceSize = GetUtf8CharLenght(CharToReplace);
		const auto ReplacementSize = GetUtf8CharLenght(ReplacementChar);

		if (ToReplaceSize == ReplacementSize) // Trivial replacement
		{
			// TODO
		}
		else if (ToReplaceSize < ReplacementSize) // 
		{
			// TODO
		}
		else /* if (ToReplaceSize > ReplacementSize) */ // Replace and move following bytes back
		{
			// TODO
		}
	}

	// utf_char spezialization-implementation for Utf8
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf8>::utf_char(utf8_bytes InChar) noexcept
		: Char(InChar)
	{
	}

	template<typename char_type, typename>
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf8>::utf_char(const char_type* SingleCharString) noexcept
		: utf_char<UtfEncodingType::Utf8>(ParseUtf8CharFromStr(SingleCharString))
	{
	}

	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf8>& utf_char<UtfEncodingType::Utf8>::operator=(utf8_bytes inBytse) noexcept
	{
		Char.Codepoints[0] = inBytse.Codepoints[0];
		Char.Codepoints[1] = inBytse.Codepoints[1];
		Char.Codepoints[2] = inBytse.Codepoints[2];
		Char.Codepoints[3] = inBytse.Codepoints[3];

		return *this;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf8>::operator==(utf_char8 Other) const noexcept
	{
		return Char == Other.Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		utf_cp8_t& utf_char<UtfEncodingType::Utf8>::operator[](const uint8_t Index)
	{
#ifdef _DEBUG
		if (Index >= 0x4)
			throw std::out_of_range("Index was greater than 4!");
#endif // _DEBUG
		return Char.Codepoints[Index];
	}

	UTF_CONSTEXPR UTF_NODISCARD
		const utf_cp8_t& utf_char<UtfEncodingType::Utf8>::operator[](const uint8_t Index) const
	{
#ifdef _DEBUG
		if (Index >= 0x4)
			throw std::out_of_range("Index was greater than 4!");
#endif // _DEBUG
		return Char.Codepoints[Index];
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf8>::operator!=(utf_char8 Other) const noexcept
	{
		return Char != Other.Char;
	}
	
	UTF_CONSTEXPR UTF_NODISCARD
		utf_char8 utf_char<UtfEncodingType::Utf8>::GetAsUtf8() const noexcept
	{
		return *this;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char16 utf_char<UtfEncodingType::Utf8>::GetAsUtf16() const noexcept
	{
		return Utf8BytesToUtf16(*this);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char32 utf_char<UtfEncodingType::Utf8>::GetAsUtf32() const noexcept
	{
		return Utf8BytesToUtf32(*this);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf8_bytes utf_char<UtfEncodingType::Utf8>::Get() const
	{
		return Char;
	}
	
	UTF_CONSTEXPR UTF_NODISCARD
		UtfEncodingType utf_char<UtfEncodingType::Utf8>::GetEncoding() const noexcept
	{
		return UtfEncodingType::Utf8;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		uint8_t utf_char<UtfEncodingType::Utf8>::GetNumCodepoints() const noexcept
	{
		return GetUtf8CharLenght(Char.Codepoints[0]);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		/* static */ uint8_t utf_char<UtfEncodingType::Utf8>::GetCodepointSize() noexcept
	{
		return 0x1;
	}



	// utf_char spezialization-implementation for Utf8
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf16>::utf_char(utf16_pair InChar) noexcept
		: Char(InChar)
	{
	}

	template<typename char_type, typename>
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf16>::utf_char(const char_type* SingleCharString) noexcept
		: utf_char<UtfEncodingType::Utf16>(ParseUtf16CharFromStr(SingleCharString))
	{
	}

	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf16>& utf_char<UtfEncodingType::Utf16>::operator=(utf16_pair inBytse) noexcept
	{
		Char.Upper = inBytse.Upper;
		Char.Lower = inBytse.Lower;

		return *this;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf16>::operator==(utf_char16 Other) const noexcept
	{
		return Char == Other.Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf16>::operator!=(utf_char16 Other) const noexcept
	{
		return Char != Other.Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		utf_char8 utf_char<UtfEncodingType::Utf16>::GetAsUtf8() const noexcept
	{
		return Utf16PairToUtf8Bytes(Char);
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		utf_char16 utf_char<UtfEncodingType::Utf16>::GetAsUtf16() const noexcept
	{
		return Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		utf_char32 utf_char<UtfEncodingType::Utf16>::GetAsUtf32() const noexcept
	{
		return Utf16PairToUtf32(Char);
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		utf16_pair utf_char<UtfEncodingType::Utf16>::Get() const noexcept
	{
		return Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		UtfEncodingType utf_char<UtfEncodingType::Utf16>::GetEncoding() const noexcept
	{
		return UtfEncodingType::Utf16;
	}

	UTF_CONSTEXPR UTF_NODISCARD 
		uint8_t utf_char<UtfEncodingType::Utf16>::GetNumCodepoints() const noexcept
	{
		return GetUtf16CharLenght(Char.Upper);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		/* static */ uint8_t utf_char<UtfEncodingType::Utf16>::GetCodepointSize() noexcept
	{
		return 0x2;
	}



	// utf_char spezialization-implementation for Utf32
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf32>::utf_char(utf_cp32_t InChar) noexcept
		: Char(InChar)
	{
	}

	template<typename char_type, typename>
	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf32>::utf_char(const char_type* SingleCharString) noexcept
		: Char(*SingleCharString)
	{
	}

	UTF_CONSTEXPR utf_char<UtfEncodingType::Utf32>& utf_char<UtfEncodingType::Utf32>::operator=(utf_cp32_t inBytse) noexcept
	{
		Char = inBytse;
		return *this;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf32>::operator==(utf_char32 Other) const noexcept
	{
		return Char == Other.Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		bool utf_char<UtfEncodingType::Utf32>::operator!=(utf_char32 Other) const noexcept
	{
		return Char != Other.Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char8 utf_char<UtfEncodingType::Utf32>::GetAsUtf8() const noexcept
	{
		return Utf32ToUtf8Bytes(Char);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char16 utf_char<UtfEncodingType::Utf32>::GetAsUtf16() const noexcept
	{
		return Utf32ToUtf16Pair(Char);
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_char32 utf_char<UtfEncodingType::Utf32>::GetAsUtf32() const noexcept
	{
		return Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		utf_cp32_t utf_char<UtfEncodingType::Utf32>::Get() const noexcept
	{
		return Char;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		UtfEncodingType utf_char<UtfEncodingType::Utf32>::GetEncoding() const noexcept
	{
		return UtfEncodingType::Utf32;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		uint8_t utf_char<UtfEncodingType::Utf32>::GetNumCodepoints() const noexcept
	{
		return 0x1;
	}

	UTF_CONSTEXPR UTF_NODISCARD
		/* static */ uint8_t utf_char<UtfEncodingType::Utf32>::GetCodepointSize() noexcept
	{
		return 0x4;
	}
}

#undef UTF_CONSTEXPR
#undef UTF_CONSTEXPR14
#undef UTF_CONSTEXPR17
#undef UTF_CONSTEXPR20
#undef UTF_CONSTEXPR23
#undef UTF_CONSTEXPR26


// Restore all warnings suppressed for the UTF-N implementation
#if (defined(_MSC_VER))
#pragma warning (pop)
#elif (defined(__CLANG__) || defined(__GNUC__))
#pragma GCC diagnostic pop
#endif // Warnings)";

	WriteFileEnd(UnicodeLib, EFileType::UnicodeLib);
}
