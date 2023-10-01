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
private:
	static inline MemberNode MakeMemberNode(const char* Type, const char* Name, int32 Offset, int32 Size, int32 ArrayDim = 0x1, EClassCastFlags Flags = EClassCastFlags::None, uint8 FieldMask = 0xFF)
	{
		MemberNode Member;
		Member.Type = Type;
		Member.Name = Name;
		Member.Offset = Offset;
		Member.Size = Size;
		Member.ArrayDim = ArrayDim;
		Member.ObjFlags = EObjectFlags::NoFlags;
		Member.PropertyFlags = EPropertyFlags::None;
		Member.CastFlags = Flags;
		Member.bIsBitField = FieldMask != 0xFF;
		Member.BitFieldIndex = [=]() -> uint8 { if (FieldMask == 0xFF) { return 0xFF; } int Idx = 0;  while ((FieldMask >> Idx) != 0x1) { Idx++; } return Idx; }();
		Member.BitMask = FieldMask;
		Member.UnrealProperty = nullptr;

		return Member;
	}

public:
	static void TestAll()
	{
		TestMakeMember();
		TestBytePadding();
		TestBitPadding();

		GenerateStructTest();

		std::cout << std::endl;
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

		MemberNode FirstMember = MakeMemberNode("int32", "OutputIndex", 0x0, 0x4);
		MemberNode SecondMember = MakeMemberNode("class FName", "ExpressionName", 0x4, 0x8);

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

		StructNode VectorNode;
		VectorNode.RawName = "Vector";
		VectorNode.PrefixedName = "FVector";
		VectorNode.UniqueNamePrefix = "CoreUObject";
		VectorNode.FullName = "ScriptStruct CoreUObject.Vector";

		VectorNode.Size = 0xC;
		VectorNode.SuperSize = 0x0;

		VectorNode.UnrealStruct = nullptr;
		VectorNode.Super = nullptr;
		VectorNode.Members = { MakeMemberNode("float", "X", 0x0, 0x4), MakeMemberNode("float", "Y", 0x4, 0x8), MakeMemberNode("float", "Z", 0x8, 0xC) };
		VectorNode.Functions = {};

		// FUNCTIONS
	}
};