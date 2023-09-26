#pragma once
#include "CppGenerator.h"
#include <cassert>

#define CHECK_RESULT(Result, Expected) \
if (Result != Expected) \
{ \
	std::cout << __FUNCTION__ << ":\nResult: \n" << Result << "\nExpected: \n" << Expected << std::endl; \
} \
else \
{ \
	std::cout << __FUNCTION__ << ": Everything is fine!" << std::endl; \
}

class CppGeneratorTest
{
public:
	static void TestAll()
	{
		TestMakeMember();
		TestBytePadding();
		TestBitPadding();

		GenerateStructTest();
	}

	static void TestMakeMember()
	{
		std::string Result = CppGenerator::MakeMemberString("uint32", "ThisIsNumber", "Very number, yes. (flags, and stuff)");
		std::string Expected = "	uint32                                       ThisIsNumber;                                      // Very number, yes. (flags, and stuff)\n";

		CHECK_RESULT(Result, Expected);
	}

	static void TestBytePadding()
	{
		std::string Result = CppGenerator::GenerateBytePadding(0x60, 0x800, "Something unreflected I guess.");
		std::string Expected = "	uint8                                        Pad_0[0x800];                                      // Something unreflected I guess.\n";

		CHECK_RESULT(Result, Expected);
	}

	static void TestBitPadding()
	{
		std::string Result = CppGenerator::GenerateBitPadding(0x60, 0x5, "Idk, just some bits.");
		std::string Expected = "	uint8                                        BitPad_0 : 5;                                      // Idk, just some bits.\n";

		CHECK_RESULT(Result, Expected);
	}

	static void GenerateStructTest()
	{
		StructNode SuperNode;
		SuperNode.RawName = "MaterialInput";
		SuperNode.PrefixedName = "FMaterialInput";
		SuperNode.UniqueNamePrefix = "";
		SuperNode.FullName = "ScriptStruct Engine.MaterialInput";

		SuperNode.Size = 0xC;
		SuperNode.SuperSize = 0x0;

		SuperNode.UnrealStruct = nullptr;
		SuperNode.Super = nullptr;

		MemberNode FirstMember;
		FirstMember.Type = "int32";
		FirstMember.Name = "OutputIndex";
		FirstMember.Offset = 0x0;
		FirstMember.Size = 0x4;
		FirstMember.ArrayDim = 0x1;
		FirstMember.ObjFlags = EObjectFlags::NoFlags;
		FirstMember.PropertyFlags = EPropertyFlags::None;
		FirstMember.CastFlags = EClassCastFlags::IntProperty;
		FirstMember.bIsBitField = false;
		FirstMember.BitFieldIndex = 0x0;
		FirstMember.BitMask = 0xFF;
		FirstMember.UnrealProperty = nullptr;

		MemberNode SecondMember;
		SecondMember.Type = "class FName";
		SecondMember.Name = "ExpressionName";
		SecondMember.Offset = 0x4;
		SecondMember.Size = 0x8;
		SecondMember.ArrayDim = 0x1;
		SecondMember.ObjFlags = EObjectFlags::NoFlags;
		SecondMember.PropertyFlags = EPropertyFlags::None;
		SecondMember.CastFlags = EClassCastFlags::NameProperty;
		SecondMember.bIsBitField = false;
		SecondMember.BitFieldIndex = 0x0;
		SecondMember.BitMask = 0xFF;
		SecondMember.UnrealProperty = nullptr;

		SuperNode.Members = { std::move(FirstMember), std::move(SecondMember) };
		std::sort(SuperNode.Members.begin(), SuperNode.Members.end(), [](const MemberNode& L, const MemberNode& R) { return L.Offset < R.Offset; });
		SuperNode.Functions = {};

		StructNode Node;
		Node.RawName = "ScalarMaterialInput";
		Node.PrefixedName = "FScalarMaterialInput";
		Node.UniqueNamePrefix = "";
		Node.FullName = "ScriptStruct Engine.ScalarMaterialInput";

		Node.Size = 0xC;
		Node.SuperSize = 0xC;

		Node.UnrealStruct = nullptr;
		Node.Super = &SuperNode;
		Node.Members = {};
		Node.Functions = {};

		std::stringstream OutStream;
		CppGenerator::GenerateStruct(OutStream, Node);
		std::string Result = OutStream.str();
		std::string Expected = 
R"(
// 0x0 (0xC - 0xC)
// ScriptStruct Engine.ScalarMaterialInput
struct FScalarMaterialInput : public FMaterialInput
{
};
)";

		CHECK_RESULT(Result, Expected);

		std::stringstream OutStream2;
		CppGenerator::GenerateStruct(OutStream2, SuperNode);
		std::string Result2 = OutStream2.str();
		std::string Expected2 =
			R"(
// 0xC (0xC - 0x0)
// ScriptStruct Engine.MaterialInput
struct FMaterialInput
{
public:
	int32                                        OutputIndex;                                       // 0x0(0x4)()
	class FName                                  ExpressionName;                                    // 0x4(0x8)()
};
)";
		CHECK_RESULT(Result2, Expected2);
	}
};