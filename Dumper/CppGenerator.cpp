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

std::string CppGenerator::GenerateMembers(const std::vector<MemberNode>& Members, int32 SuperSize)
{
	constexpr int EstimatedCharactersPerLine = 0x80;

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

		if (Member.Offset >= PrevPropertyEnd && bLastPropertyWasBitField && PrevBoolPropertyBit != 9)
		{
			OutMembers += GenerateBitPadding(Member.Offset, 9 - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");
		}

		if (Member.Offset > PrevPropertyEnd)
		{
			OutMembers += GenerateBytePadding(PrevPropertyEnd, Member.Offset - PrevPropertyEnd, "Fixing Size After Last Property  [ Dumper-7 ]");
		}

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
		OutMembers += MakeMemberString(Member.Type, Member.Name, std::move(Comment));
	}

	return OutMembers;
}

void CppGenerator::GenerateStruct(StreamType& StructFile, const StructNode& Struct)
{
	std::string UniqueName = Struct.UniqueNamePrefix.empty() ? Struct.PrefixedName : Struct.UniqueNamePrefix + "::" + Struct.PrefixedName;
	std::string UniqueSuperName;

	if (Struct.Super)
		UniqueSuperName = Struct.Super->UniqueNamePrefix.empty() ? Struct.Super->PrefixedName : Struct.Super->UniqueNamePrefix + "::" + Struct.Super->PrefixedName;


	StructFile << std::format(R"(
// 0x{:X} (0x{:X} - 0x{:X})
// {}
struct {}{}
{{
)", Struct.Size - Struct.SuperSize, Struct.Size, Struct.SuperSize, Struct.FullName, UniqueName, Struct.Super ? (" : public " + UniqueSuperName) : "");

	if (!Struct.Members.empty())
		StructFile << "public:\n" + GenerateMembers(Struct.Members, Struct.SuperSize);

	StructFile << "};\n";

//	StructFile << std::format(
//		R"(
//// 0x{:X} (0x{:X} - 0x{:X})
//// {}
//struct {}{}
//{{
//public:
//{}
//}};)", Struct.Size - Struct.SuperSize, Struct.Size, Struct.SuperSize, Struct.FullName, 
//	   UniqueName, (Struct.Super ? ": public " + UniqueSuperName : ""), GenerateMembers(Struct.Members, Struct.SuperSize));
}

void CppGenerator::GenerateClass(StreamType& ClassFile, const StructNode& Class)
{

}

void CppGenerator::GenerateFunction(StreamType& FunctionFile, std::ofstream& ParamFile, const FunctionNode& Function)
{

}

void CppGenerator::Generate(const DependencyManager& Dependencies)
{

}

void CppGenerator::InitPredefinedMembers()
{

}

void CppGenerator::InitPredefinedFunctions()
{

}