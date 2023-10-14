#include "CppGenerator.h"



std::string CppGenerator::MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment)
{
	return std::format("\t{:{}}{:{}} // {}\n", Type, 45, Name + ";", 50, std::move(Comment));
}

std::string CppGenerator::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return MakeMemberString("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), std::move(Reason));
}

std::string CppGenerator::GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 BitPadNum = 0;

	return MakeMemberString("uint8", std::format("BitPad_{:X} : {:X}", BitPadNum++, PadSize), std::move(Reason));
}

std::string CppGenerator::GenerateMembers(const HashStringTable& NameTable, const std::vector<MemberNode>& Members, int32 SuperSize)
{
	constexpr int EstimatedCharactersPerLine = 0x80;
	constexpr int NumBitsInBytePlusOne = 0x9;

	if (Members.empty())
		return "";

	std::string OutMembers;
	OutMembers.reserve(Members.size() * EstimatedCharactersPerLine);

	bool bLastPropertyWasBitField = false;

	int PrevPropertyEnd = SuperSize;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	// Iterate using indices becauase i gets modified by GetMemberTypeString to allow skipping "inner" properties. eg. Members[i] == ArrayProperty -> Members[i+1] == ArrayProperty::Inner
	for (int i = 0; i < Members.size(); i++)
	{
		const MemberNode& Member = Members[i];

		std::string Comment = std::format("0x{:04X}(0x{:04X})({})", Member.Offset, Member.Size, StringifyPropertyFlags(Member.PropertyFlags));

		if (Member.Offset >= PrevPropertyEnd && bLastPropertyWasBitField && PrevBoolPropertyBit != NumBitsInBytePlusOne)
			OutMembers += GenerateBitPadding(Member.Offset, NumBitsInBytePlusOne - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

		if (Member.Offset > PrevPropertyEnd)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, Member.Offset - PrevPropertyEnd, "Fixing Size After Last Property  [ Dumper-7 ]");

		bLastPropertyWasBitField = Member.bIsBitField;

		if (Member.bIsBitField)
		{
			Comment = std::format("Mask: 0x{:02X}, PropSize: 0x{:04X} {}", Member.BitMask, Member.Size, Comment);

			if (PrevBoolPropertyEnd < Member.Offset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < Member.BitFieldIndex)
				OutMembers += GenerateBitPadding(Member.Offset, Member.BitFieldIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

			PrevBoolPropertyBit = Member.BitFieldIndex + 1;
			PrevBoolPropertyEnd = Member.Offset;

			bLastPropertyWasBitField = true;
		}

		PrevPropertyEnd = Member.Offset + Member.Size;

		std::string MemberName = NameTable[Member.Name].GetUniqueName();
		
		if (Member.ArrayDim > 1)
			MemberName += std::format("[0x{:X}]", Member.ArrayDim);

		OutMembers += MakeMemberString(GetMemberTypeString(NameTable, Member, Members, i /* gets modified */), MemberName, std::move(Comment));
	}

	return OutMembers;
}

std::string CppGenerator::GenerateFunctionInClass(const HashStringTable& NameTable, const std::vector<FunctionNode>& Functions)
{
	return "CppGenerator::GenerateFunctionInClass";
}

void CppGenerator::GenerateStruct(const HashStringTable& NameTable, StreamType& StructFile, const StructNode& Struct)
{
	std::string UniqueName = GetStructPrefixedName(NameTable, Struct);
	std::string UniqueSuperName;

	if (Struct.Super)
		UniqueSuperName = GetStructPrefixedName(NameTable, *Struct.Super);

	StructFile << std::format(R"(
// 0x{:04X} (0x{:04X} - 0x{:04X})
// {}
struct {}{}
{{
)", Struct.Size - Struct.SuperSize, Struct.Size, Struct.SuperSize, Struct.GetFullNameEntry(NameTable).GetFullName(), UniqueName, Struct.Super ? (" : public " + UniqueSuperName) : "");

	const bool bHasMembers = !Struct.Members.empty();
	const bool bHasFunctions = !Struct.Functions.empty();

	if (bHasMembers || bHasFunctions)
		StructFile << "public:\n";

	if (bHasMembers)
	{
		StructFile << GenerateMembers(NameTable, Struct.Members, Struct.SuperSize);

		if (bHasFunctions)
			StructFile << "\n\n";
	}

	if (bHasFunctions)
		StructFile << "";

	StructFile << "};\n";
}

void CppGenerator::GenerateClass(const HashStringTable& NameTable, StreamType& ClassFile, const StructNode& Class)
{

}

void CppGenerator::GenerateFunctionInCppFile(const HashStringTable& NameTable, StreamType& FunctionFile, std::ofstream& ParamFile, const FunctionNode& Function)
{

}

std::string CppGenerator::GetStructPrefixedName(const HashStringTable& NameTable, const StructNode& Struct)
{
	const StringEntry& UniqueNameEntry = Struct.GetPrefixedNameEntry(NameTable);

	return UniqueNameEntry.IsUnique() ? UniqueNameEntry.GetUniqueName() : Struct.GetPackageNameEntry(NameTable).GetRawName() + "::" + UniqueNameEntry.GetUniqueName();
}

void CppGenerator::Generate(const HashStringTable& NameTable, const DependencyManager& Dependencies)
{

}

void CppGenerator::InitPredefinedMembers()
{

}

void CppGenerator::InitPredefinedFunctions()
{

}

std::string CppGenerator::GetMemberTypeString(const HashStringTable& NameTable, const MemberNode& CurrentNode, const std::vector<MemberNode>& Nodes, int32& InOutNodeIndex)
{
	static auto GetInnnerPropertyType = [](const HashStringTable& NameTable, const std::vector<MemberNode>& Nodes, int32& InOutNodeIndex)-> std::string
	{
		/* Modifies index to retreive inner properties such as UEArrayProperty::InnerProperty*/
		InOutNodeIndex++;
		const MemberNode& InnerProperty = Nodes[InOutNodeIndex];
		return GetMemberTypeString(NameTable, InnerProperty, Nodes, InOutNodeIndex);
	};

	std::string CustomTypeNameNamespace = CurrentNode.CustomTypeNameNamespace ? NameTable[CurrentNode.CustomTypeNameNamespace].GetUniqueName() + "::" : "";
	std::string CustomTypeName = CustomTypeNameNamespace + (CurrentNode.CustomTypeName ? NameTable[CurrentNode.CustomTypeName].GetUniqueName() : "");


	if (CurrentNode.CastFlags & EClassCastFlags::ByteProperty)
	{
		return CurrentNode.CustomTypeName ? CustomTypeName : "uint8";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::ClassProperty)
	{
		if (CurrentNode.PropertyFlags & EPropertyFlags::UObjectWrapper)
			return std::format("TSubclassOf<class {}>", CustomTypeName);

		return "class UClass*";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::BoolProperty)
	{
		return CurrentNode.bIsBitField ? "uint8" : "bool";
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::StructProperty)
	{
		return std::format("struct {}", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::ArrayProperty)
	{
		return std::format("TArray<{}>", GetInnnerPropertyType(NameTable, Nodes, InOutNodeIndex));
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::WeakObjectProperty)
	{
		return std::format("TWeakObjectPtr<class {}>", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::LazyObjectProperty)
	{
		return std::format("TLazyObjectPtr<class {}>", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::SoftClassProperty)
	{
		return std::format("TSoftClassPtr<class {}>", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::SoftObjectProperty)
	{
		return std::format("TSoftObjectPtr<class {}>", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::ObjectProperty)
	{
		return std::format("class {}*", CustomTypeName);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::MapProperty)
	{
		std::string KeyPropertyString = GetInnnerPropertyType(NameTable, Nodes, InOutNodeIndex);
		std::string ValuePropertyString = GetInnnerPropertyType(NameTable, Nodes, InOutNodeIndex);
		
		return std::format("TMap<{}, {}>", KeyPropertyString, ValuePropertyString);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::SetProperty)
	{
		std::string KeyPropertyString = GetInnnerPropertyType(NameTable, Nodes, InOutNodeIndex);
		return std::format("TSet<{}>", KeyPropertyString);
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::EnumProperty)
	{
		return CurrentNode.bIsEnumClass ? std::format("enum class {}", CustomTypeName) : CustomTypeName;
	}
	else if (CurrentNode.CastFlags & EClassCastFlags::InterfaceProperty)
	{
		return std::format("TScriptInterface<class {}>", CustomTypeName);
	}
	else
	{
		return NameTable[CurrentNode.PropertyClassName].GetUniqueName() + "_";
	}
}