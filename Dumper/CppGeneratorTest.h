#pragma once
#include "CppGenerator.h"
#include "HashStringTable.h"
#include "TestBase.h"

#include <cassert>


class CppGeneratorTest : protected TestBase
{
private:
	static inline std::vector<PredefinedFunction> TestFunctions = {
		PredefinedFunction{
			"IsReelPawn",
			"bool IsReelPawn(bool bIsGuaranteedToBePawn)",
			"bool APawn::IsReelPawn(bool bIsGuaranteedToBePawn)",
			"\t{\n\t\treturn bIsGuaranteedToBePawn + 4;\n\t}"
			, false, false
		},
		PredefinedFunction{
			"GetActors",
			"TArray<class AActor*>* GetActors(int a1)",
			"TArray<class AActor*>* APawn::GetActors(int a1)",
			"\t{\n\t\treturn reinterpret_cast<TArray<class AActor*>*>(&a2);\n\t}"
			, true, false
		},
		PredefinedFunction{
			"GetNullPtr",
			"std::nullptr_t GetNullPtr()",
			"",
			"\t{\n\t\treturn nullptr;\n\t}"
			, false, true
		},
		PredefinedFunction{
			"StaticTestFunc",
			"void StaticTestFunc(bool* bIsGuaranteedToBePawn)",
			"",
			"\t{\n\t\t*bIsGuaranteedToBePawn = true;\n\t}"
			, true, true
		}
	};

public:
	template<bool bDoDebugPrinting = false>
	static void TestAll()
	{
		TestPredefStructGeneration<bDoDebugPrinting>();
		TestUnrealStructGeneration<bDoDebugPrinting>();

		std::cout << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestPredefStructGeneration()
	{
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
		   TestFunctions
		};

		std::sort(TestStruct.Properties.begin(), TestStruct.Properties.end(), ComparePredefinedMembers);
		std::sort(TestStruct.Functions.begin(), TestStruct.Functions.end(), ComparePredefinedFunctions);

		MemberManager::InitMemberNameCollisions();
		StructManager::Init();

		CppGenerator::GenerateStruct(std::cout, &TestStruct);
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUnrealStructGeneration()
	{
		MemberManager::InitMemberNameCollisions();
		StructManager::Init();

		std::sort(TestFunctions.begin(), TestFunctions.end(), ComparePredefinedFunctions);

		UEStruct FTransform = ObjectArray::FindObjectFast<UEStruct>("Transform");

		PredefinedMemberLookupMapType PredefMemberLookup;
		PredefMemberLookup[FTransform.GetIndex()] = { { }, TestFunctions };

		MemberManager::SetPredefinedMemberLookupPtr(&PredefMemberLookup);

		CppGenerator::GenerateStruct(std::cout, FTransform);
	}
};