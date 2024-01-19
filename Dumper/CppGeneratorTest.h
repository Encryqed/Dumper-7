#pragma once
#include "CppGenerator.h"
#include "HashStringTable.h"
#include "TestBase.h"
#include "PackageManager.h"

#include <cassert>


class CppGeneratorTest : protected TestBase
{
private:
	static inline std::vector<PredefinedFunction> TestFunctions = {
		PredefinedFunction {
			"// Function tells if if this pawn is reel",
			"",
			"bool",
			"IsReelPawn(bool bIsGuaranteedToBePawn)",
			"",
			"{\n\treturn bIsGuaranteedToBePawn + 4;\n}"
			, false, false, false
		},
		PredefinedFunction {
			"/* Gets a bunch of actors or smoething, idk */",
			"",
			"TArray<class AActor*>*",
			"GetActors(int a1)",
			"",
			"{\n\treturn reinterpret_cast<TArray<class AActor*>*>(&a2);\n}"
			, true, false, false
		},
		PredefinedFunction {
			"",
			"",
			"std::nullptr_t",
			"GetNullPtr()",
			"",
			"{\n\treturn nullptr;\n}"
			, false, false, true
		},
		PredefinedFunction {
			"",
			"",
			"std::nullptr_t",
			"GetNullPtrConst()",
			"",
			"{\n\treturn nullptr;\n}"
			, false, true, true
		},
		PredefinedFunction {
			"",
			"",
			"void",
			"StaticTestFunc(bool* bIsGuaranteedToBePawn)",
			"",
			"{\n\t*bIsGuaranteedToBePawn = true;\n}"
			, true, false, true
		}
	};

	static inline std::ofstream ClassFile;
	static inline std::ofstream StructFile;
	static inline std::ofstream FunctionFile;
	static inline std::ofstream ParamFile;

	static inline std::ofstream NameCollisionInl;

private:
	static void InitTestVariables()
	{
		static bool bDidInit = false;

		if (bDidInit)
			return;

		MemberManager::Init();
		StructManager::Init();
		EnumManager::Init();
		PackageManager::Init();

		std::sort(TestFunctions.begin(), TestFunctions.end(), ComparePredefinedFunctions);

		fs::path BasePath("C:/Users/savek/Documents/GitHub/Fortnite-Dumper-7/SDKTest");

		ClassFile = std::ofstream(BasePath / "CPP_classes.hpp", std::ios::trunc);
		StructFile = std::ofstream(BasePath / "CPP_structs.hpp", std::ios::trunc);
		FunctionFile = std::ofstream(BasePath / "CPP_functions.cpp", std::ios::trunc);
		ParamFile = std::ofstream(BasePath / "CPP_params.hpp", std::ios::trunc);

		NameCollisionInl = std::ofstream(BasePath / "NameCollision.inl", std::ios::trunc);

		bDidInit = true;
	}

public:
	template<bool bDoDebugPrinting = false>
	static void TestAll()
	{
		TestPredefStructGeneration<bDoDebugPrinting>();
		TestUnrealStructGeneration<bDoDebugPrinting>();
		TestUnrealClassGeneration<bDoDebugPrinting>();
		TestUnrealEnumGeneration<bDoDebugPrinting>();
		TestNameCollisionInlCreation<bDoDebugPrinting>();

		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestPredefStructGeneration()
	{
		InitTestVariables();

		PredefinedStruct TestStruct = {
			"FField",
			0x50,
			0x8,
			false,
			false,
			true,
			nullptr,
			{
			   PredefinedMember{
				   "void*", "Vft",
				   Off::FField::Vft,
				   sizeof(void*), 0x1, alignof(void*),
				   false, false, 0xFF
			   },
			   PredefinedMember{
				   "class FFieldClass*", "Class",
				   Off::FField::Class,
				   sizeof(void*), 0x1, alignof(void*),
				   false, false, 0xFF
			   },
			   PredefinedMember{
				   "FFieldVariant", "Owner",
				   Off::FField::Owner,
				   2 * sizeof(void*), 0x1, alignof(void*),
				   false, false, 0xFF
			   },
			   PredefinedMember{
				   "int32", "Flags",
				   Off::FField::Flags,
				   sizeof(int32), 0x1, alignof(int32),
				   false, false, 0xFF
			   },
			},
			{
				TestFunctions[0],
				TestFunctions[2],
				TestFunctions[3],
			}
		};

		std::sort(TestStruct.Properties.begin(), TestStruct.Properties.end(), ComparePredefinedMembers);
		std::sort(TestStruct.Functions.begin(), TestStruct.Functions.end(), ComparePredefinedFunctions);

		CppGenerator::GenerateStruct(&TestStruct, ClassFile, FunctionFile, ParamFile);
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUnrealStructGeneration()
	{
		InitTestVariables();

		UEStruct FTransform = ObjectArray::FindObjectFast<UEStruct>("Transform");

		PredefinedMemberLookupMapType PredefMemberLookup;
		PredefMemberLookup[FTransform.GetIndex()] = { { }, TestFunctions };

		MemberManager::SetPredefinedMemberLookupPtr(&PredefMemberLookup);

		CppGenerator::GenerateStruct(FTransform, StructFile, FunctionFile, ParamFile);
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUnrealClassGeneration()
	{
		InitTestVariables();

		MemberManager::SetPredefinedMemberLookupPtr(&CppGenerator::PredefinedMembers);

		CppGenerator::GenerateStruct(ObjectArray::FindClassFast("Pawn"), ClassFile, FunctionFile, ParamFile);
		CppGenerator::GenerateStruct(ObjectArray::FindClassFast("KismetSystemLibrary"), ClassFile, FunctionFile, ParamFile);
		CppGenerator::GenerateStruct(ObjectArray::FindClassFast("KismetMathLibrary"), ClassFile, FunctionFile, ParamFile);
		CppGenerator::GenerateStruct(ObjectArray::FindClassFast("ABPI_WeaponAnimLayer_C"), ClassFile, FunctionFile, ParamFile);
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUnrealEnumGeneration()
	{
		InitTestVariables();

		MemberManager::SetPredefinedMemberLookupPtr(&CppGenerator::PredefinedMembers);

		CppGenerator::GenerateEnum(ObjectArray::FindObjectFast<UEEnum>("ENetRole"), StructFile);
		CppGenerator::GenerateEnum(ObjectArray::FindObjectFast<UEEnum>("ESlateColorStylingMode"), StructFile);
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestNameCollisionInlCreation()
	{
		InitTestVariables();

		CppGenerator::GenerateNameCollisionsInl(NameCollisionInl);
	}
};