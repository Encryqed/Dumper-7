#pragma once
#include <unordered_set>
#include "MemberManager.h"
#include "MemberWrappers.h"
#include "TestBase.h"

class MemberManagerTest : protected TestBase
{
private:
	static inline void SetBoolIfFailed(bool& BoolToSet, bool bSucceded)
	{
		BoolToSet = BoolToSet && bSucceded;
	}

public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestInit<bDoDebugPrinting>();
		TestMemberIterator<bDoDebugPrinting>();
		TestFunctionIterator<bDoDebugPrinting>();

		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInit()
	{
		MemberManager::Init();

		size_t FirstInitSize = MemberManager::MemberNames.NameInfos.size();

		MemberManager::Init();
		MemberManager::Init();
		MemberManager::Init();

		bool bSuccededTestWithoutError = MemberManager::MemberNames.NameInfos.size() == FirstInitSize;

		PrintDbgMessage<bDoDebugPrinting>("{} --> NameInfos.size(): 0x{:X} -> 0x{:X}", __FUNCTION__, FirstInitSize, MemberManager::MemberNames.NameInfos.size());
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}


	template<bool bDoDebugPrinting = false>
	static inline void TestMemberIterator()
	{
		MemberManager::Init();

		PredefinedMemberLookupMapType PredefinedMembers;

		UEClass ULevel = ObjectArray::FindClassFast("Level");
		const int32 ULevelIndex = ULevel.GetIndex();

		std::vector<PredefinedMember>& ULevelMembers = PredefinedMembers[ULevelIndex].Members;

		ULevelMembers =
		{
			PredefinedMember{ "", "void*", "StaticPtr", 0x00, 0x08, 0x01, 0x08, true, false, false, 0xFF },
			PredefinedMember{ "", "TArray<class AActor*>", "Actors", Off::ULevel::Actors, 0x10, 0x01, 0x08, false, false, false, 0xFF },
			PredefinedMember{ "", "std::nullptr_t", "MadeUpShit", 0x140, 0x08, 0x03, 0x08, false, false, false, 0xFF },
			PredefinedMember{ "", "uint8", "bIsBitfield", 0x30, 0x01, 0x01, 0x01, false, false, true, 0x4 }
		};

		std::sort(ULevelMembers.begin(), ULevelMembers.end(), ComparePredefinedMembers);

		StructManager::Init();
		MemberManager::SetPredefinedMemberLookupPtr(&PredefinedMembers);

		MemberManager Members(ULevel);

		int32 PrevMemberEnd = -1;
		bool bEncounteredNonStaticMembersBefore = false;
		const PropertyWrapper* Prev = nullptr;

		int32 PrevBoolPropertyEnd = 0x0;
		int32 PrevBoolPropertyBit = 1;
		bool bIsPrevMemberBitField = false;

		bool bSuccededTestWithoutError = true;

		for (const PropertyWrapper& Wrapper : Members.IterateMembers())
		{
			bEncounteredNonStaticMembersBefore = !Wrapper.IsStatic() && !bEncounteredNonStaticMembersBefore;

			if (PrevMemberEnd > Wrapper.GetOffset() && !Wrapper.IsStatic())
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on member '{}' [bIsUnrealProperty = {}; ArrayDim = 0x{:X}]. Offset 0x{:X} was lower than last member [0x{:X}]\n", Wrapper.GetName(), Wrapper.IsUnrealProperty(), Wrapper.GetArrayDim(), Wrapper.GetOffset(), PrevMemberEnd);
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}

			if (bEncounteredNonStaticMembersBefore && Wrapper.IsStatic())
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on member '{}' [bIsUnrealProperty = {}; ArrayDim = 0x{:X}]. Static member was encountered after non-static member. Check sorting functions!\n", Wrapper.GetName(), Wrapper.IsUnrealProperty(), Wrapper.GetArrayDim());
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}

			PrevMemberEnd = Wrapper.GetOffset() + (!Wrapper.IsBitField() ? (Wrapper.GetSize() * Wrapper.GetArrayDim()) : 0x0);

			//PrintDbgMessage<bDoDebugPrinting>("{} {}; // 0x{:04X}, 0x:{04X}", Wrapper.IsUnrealProperty() ? Wrapper.GetUnrealProperty().GetCppType() : Wrapper.GetType(), Wrapper.GetName(), Wrapper.GetOffset(), Wrapper.GetSize());
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestFunctionIterator()
	{
		MemberManager::Init();

		PredefinedMemberLookupMapType PredefinedMembers;

		UEClass APawn = ObjectArray::FindClassFast("Pawn");
		const int32 APawnIndex = APawn.GetIndex();

		std::vector<PredefinedFunction>& APawnFunctions = PredefinedMembers[APawnIndex].Functions;

		/*
		Real Order:
			- GetActors
			- IsReelPawn
			- StaticTestFunc
			- GetNullPtr
		*/
		APawnFunctions = {
			PredefinedFunction {
				"// Function tells if if this pawn is reel",
				"",
				"bool",
				"IsReelPawn(bool bIsGuaranteedToBePawn)",
				"",
				"\t{\n\t\treturn bIsGuaranteedToBePawn + 4;\n\t}"
				, false, false, false
		},
			PredefinedFunction {
				"/* Gets a bunch of actors or smoething, idk */",
				"",
				"TArray<class AActor*>*",
				"GetActors(int a1)",
				"",
				"\t{\n\t\treturn reinterpret_cast<TArray<class AActor*>*>(&a2);\n\t}"
				, true, false, false
		},
			PredefinedFunction {
				"",
				"",
				"std::nullptr_t",
				"GetNullPtr()",
				"",
				"\t{\n\t\treturn nullptr;\n\t}"
				, false, false, true
		},
			PredefinedFunction {
				"",
				"",
				"std::nullptr_t",
				"GetNullPtrConst()",
				"",
				"\t{\n\t\treturn nullptr;\n\t}"
				, false, true, true
		},
			PredefinedFunction {
				"",
				"",
				"void",
				"StaticTestFunc(bool* bIsGuaranteedToBePawn)",
				"",
				"\t{\n\t\t*bIsGuaranteedToBePawn = true;\n\t}"
				, true, false, true
		}
		};

		std::sort(APawnFunctions.begin(), APawnFunctions.end(), ComparePredefinedFunctions);

		StructManager::Init();
		MemberManager::Init();
		MemberManager::SetPredefinedMemberLookupPtr(&PredefinedMembers);

		MemberManager Members(APawn);

		/*
		Order:
			static non-inline
			non-inline
			static inline
			inline
		*/

		bool bEncounteredStaticNonInlineFunctionBefore = false;
		bool bEncounteredNonStaticNonInlineFunctionBefore = false;
		bool bEncounteredStaticInlineFunctionBefore = false;
		bool bEncounteredNonStaticInlineFunctionBefore = false;

		bool bSuccededTestWithoutError = true;

		for (const FunctionWrapper& Wrapper : Members.IterateFunctions())
		{
			const bool bIsStaticNonInline = Wrapper.IsStatic() && !Wrapper.HasInlineBody();
			const bool bIsNonStaticNonInline = !Wrapper.IsStatic() && !Wrapper.HasInlineBody();
			const bool bIsStaticInline = Wrapper.IsStatic() && Wrapper.HasInlineBody();
			const bool bIsNonStaticInline = !Wrapper.IsStatic() && Wrapper.HasInlineBody();

			if (!bEncounteredStaticNonInlineFunctionBefore && bIsStaticNonInline)
				bEncounteredStaticNonInlineFunctionBefore = true;

			if (!bEncounteredNonStaticNonInlineFunctionBefore && bIsNonStaticNonInline)
				bEncounteredNonStaticNonInlineFunctionBefore = true;

			if (!bEncounteredStaticInlineFunctionBefore && bIsStaticInline)
				bEncounteredStaticInlineFunctionBefore = true;

			if (!bEncounteredNonStaticInlineFunctionBefore && bIsNonStaticInline)
				bEncounteredNonStaticInlineFunctionBefore = true;


			if (bIsStaticNonInline && (bEncounteredNonStaticNonInlineFunctionBefore || bEncounteredStaticInlineFunctionBefore || bEncounteredNonStaticInlineFunctionBefore))
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on function '{}' [bIsUnrealFunction = {}]. bIsStaticNonInline error. Check sorting functions", Wrapper.GetName(), !Wrapper.IsPredefined());
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}
			
			if (bIsNonStaticNonInline && (bEncounteredStaticInlineFunctionBefore || bEncounteredNonStaticInlineFunctionBefore))
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on function '{}' [bIsUnrealFunction = {}]. bIsNonStaticNonInline error. Check sorting functions", Wrapper.GetName(), !Wrapper.IsPredefined());
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}
			
			if (bIsStaticInline && (bEncounteredNonStaticInlineFunctionBefore))
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on function '{}' [bIsUnrealFunction = {}]. bIsStaticInline error. Check sorting functions", Wrapper.GetName(), !Wrapper.IsPredefined());
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}

			PrintDbgMessage<bDoDebugPrinting>("Func: {};", Wrapper.GetName());
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};