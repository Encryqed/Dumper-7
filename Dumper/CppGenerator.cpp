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

	for (const MemberNode& Member : Members)
	{
		std::string Comment = std::format("0x{:X}(0x{:X})({})", Member.Offset, Member.Size, StringifyPropertyFlags(Member.PropertyFlags));

		if (Member.Offset >= PrevPropertyEnd && bLastPropertyWasBitField && PrevBoolPropertyBit != NumBitsInBytePlusOne)
			OutMembers += GenerateBitPadding(Member.Offset, NumBitsInBytePlusOne - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

		if (Member.Offset > PrevPropertyEnd)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, Member.Offset - PrevPropertyEnd, "Fixing Size After Last Property  [ Dumper-7 ]");

		bLastPropertyWasBitField = Member.bIsBitField;

		if (Member.bIsBitField)
		{
			Comment = std::format("Mask: 0x{:X}, PropSize: 0x{:X} {}", Member.BitMask, Member.Size, Comment);

			if (PrevBoolPropertyEnd < Member.Offset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < Member.BitFieldIndex)
				OutMembers += GenerateBitPadding(Member.Offset, Member.BitFieldIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

			PrevBoolPropertyBit = Member.BitFieldIndex + 1;
			PrevBoolPropertyEnd = Member.Offset;

			bLastPropertyWasBitField = true;
		}

		PrevPropertyEnd = Member.Offset + Member.Size;
		OutMembers += MakeMemberString(GetMemberTypeString(NameTable, Member), NameTable.GetStringEntry(Member.Name).GetUniqueName(), std::move(Comment));
	}

	return OutMembers;
}

std::string CppGenerator::GenerateFunctionInClass(const HashStringTable& NameTable, const std::vector<FunctionNode>& Functions)
{

}

void CppGenerator::GenerateStruct(const HashStringTable& NameTable, StreamType& StructFile, const StructNode& Struct)
{
	std::string UniqueName = GetStructPrefixedName(NameTable, Struct);
	std::string UniqueSuperName;

	if (Struct.Super)
		UniqueSuperName = GetStructPrefixedName(NameTable, *Struct.Super);

	StructFile << std::format(R"(
// 0x{:X} (0x{:X} - 0x{:X})
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

std::string CppGenerator::GetMemberTypeString(const HashStringTable& NameTable, const MemberNode& Node)
{
	static constexpr const char* FormatStrings[static_cast<uint8>(EMappingsTypeFlags::FieldPathProperty)] = {
		/*ByteProperty = */ "uint8"
		/*BoolProperty = */ "bool"
		/*IntProperty = */ "int"
		/*FloatProperty = */ "float"
	};

	std::string InnerNamespaceName = Node.InnerTypeNameNamespace ? NameTable.GetStringEntry(Node.InnerTypeNameNamespace).GetUniqueName() + "::" : "";
	std::string InnerTypeName = InnerNamespaceName + (Node.InnerTypeName ? NameTable.GetStringEntry(Node.InnerTypeName).GetUniqueName() : "");

	Node.UnrealProperty.GetCppType();

	return std::format(FormatStrings[static_cast<uint8>(Node.TypeFlags)], InnerTypeName);

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
	else
	{
		return (GetClass().first ? GetClass().first.GetCppName() : GetClass().second.GetCppName()) + "_";;
	}
}