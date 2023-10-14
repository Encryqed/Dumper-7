#pragma once
#include "CppGenerator.h"
#include "HashStringTable.h"
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
	static inline MemberNode MakeMemberNode(HashStringTable& NameTable, EClassCastFlags Flags, const char* Type, const char* Name, int32 Offset, int32 Size, int32 ArrayDim = 0x1, const char* PropertyClassName = nullptr, uint8 FieldMask = 0xFF)
	{
		MemberNode Member;
		Member.CustomTypeName = NameTable.FindOrAdd(Type, true).first;
		Member.CustomTypeNameNamespace = HashStringTableIndex::InvalidIndex;
		Member.Name = strlen(Name) > 1 ? NameTable.FindOrAdd(Name).first : HashStringTableIndex::InvalidIndex;
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
		Member.PropertyClassName = PropertyClassName ? NameTable.FindOrAdd(PropertyClassName, true).first : HashStringTableIndex::InvalidIndex;

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
		HashStringTable NameTable;

		StructNode SuperNode;
		SuperNode.FullName = NameTable.FindOrAdd("ScriptStruct Engine.MaterialInput").first;
		SuperNode.PrefixedName = NameTable.FindOrAdd("FMaterialInput", true).first;
		SuperNode.PackageName = NameTable.FindOrAdd("Engine").first;

		SuperNode.Size = 0xC;
		SuperNode.SuperSize = 0x0;

		SuperNode.UnrealStruct = nullptr;
		SuperNode.Super = nullptr;

		MemberNode FirstMember = MakeMemberNode(NameTable, EClassCastFlags::IntProperty, "int32", "OutputIndex", 0x0, 0x4);
		MemberNode SecondMember = MakeMemberNode(NameTable, EClassCastFlags::NameProperty, "class FName", "ExpressionName", 0x4, 0x8);
		MemberNode ThirdMember = MakeMemberNode(NameTable, EClassCastFlags::ArrayProperty, "", "MyArray", 0x8, 0x10, 0x2);
		MemberNode ArrayInnerMember = MakeMemberNode(NameTable, EClassCastFlags::ObjectProperty, "AActor", "Irrelevant", 0x0, 0x0, 0x0);
		MemberNode DelegateMember = MakeMemberNode(NameTable, EClassCastFlags::MulticastInlineDelegateProperty, "", "UnknonwDelegate", 0x18, 0x1, 0x0, "FMutlicastDelegateProperty");

		SuperNode.Members = { std::move(FirstMember), std::move(SecondMember), std::move(ThirdMember), std::move(ArrayInnerMember), std::move(DelegateMember) };
		//std::sort(SuperNode.Members.begin(), SuperNode.Members.end(), [](const MemberNode& L, const MemberNode& R) { return L.Offset < R.Offset; });
		SuperNode.Functions = {};

		StructNode Node;
		Node.FullName = NameTable.FindOrAdd("ScriptStruct Engine.ScalarMaterialInput").first;
		Node.PrefixedName = NameTable.FindOrAdd("FScalarMaterialInput", true).first;
		Node.PackageName = NameTable.FindOrAdd("Engine").first;

		Node.Size = 0xC;
		Node.SuperSize = 0xC;

		Node.UnrealStruct = nullptr;
		Node.Super = &SuperNode;
		Node.Members = {};
		Node.Functions = {};

		std::stringstream OutStream;
		CppGenerator::GenerateStruct(NameTable, OutStream, Node);
		std::string Result = OutStream.str();
		std::string Expected = 
R"(
// 0x0000 (0x000C - 0x000C)
// ScriptStruct Engine.ScalarMaterialInput
struct FScalarMaterialInput : public FMaterialInput
{
};
)";

		CHECK_RESULT(Result, Expected);

		std::stringstream OutStream2;
		CppGenerator::GenerateStruct(NameTable, OutStream2, SuperNode);
		std::string Result2 = OutStream2.str();
		std::string Expected2 =
			R"(
// 0x000C (0x000C - 0x0000)
// ScriptStruct Engine.MaterialInput
struct FMaterialInput
{
public:
	int32                                        OutputIndex;                                       // 0x0000(0x0004)()
	class FName                                  ExpressionName;                                    // 0x0004(0x0008)()
	TArray<class AActor*>                        MyArray[0x2];                                      // 0x0008(0x0010)()
	FMutlicastDelegateProperty_                  UnknonwDelegate;                                   // 0x0018(0x0001)()
};
)";
		CHECK_RESULT(Result2, Expected2);

		StructNode VectorNode;
		SuperNode.FullName = NameTable.FindOrAdd("ScriptStruct CoreUObject.Vector").first;
		SuperNode.PrefixedName = NameTable.FindOrAdd("FVector", true).first;
		SuperNode.PackageName = NameTable.FindOrAdd("CoreUObject").first;

		VectorNode.Size = 0xC;
		VectorNode.SuperSize = 0x0;

		VectorNode.UnrealStruct = nullptr;
		VectorNode.Super = nullptr;
		VectorNode.Members = 
		{
			MakeMemberNode(NameTable, EClassCastFlags::FloatProperty, "float", "X", 0x0, 0x4),
			MakeMemberNode(NameTable, EClassCastFlags::FloatProperty,"float", "Y", 0x4, 0x8),
			MakeMemberNode(NameTable,  EClassCastFlags::FloatProperty, "float", "Z", 0x8, 0xC)
		};
		VectorNode.Functions = {};

		// FUNCTIONS
	}
};