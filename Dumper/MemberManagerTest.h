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
		TestNameTranslation<bDoDebugPrinting>();
		TestMemberIterator<bDoDebugPrinting>();
		TestFunctionIterator<bDoDebugPrinting>();

		std::cout << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInit()
	{
		MemberManager::InitMemberNameCollisions();

		size_t FirstInitSize = MemberManager::NameInfos.size();

		MemberManager::InitMemberNameCollisions();
		MemberManager::InitMemberNameCollisions();
		MemberManager::InitMemberNameCollisions();

		bool bSuccededTestWithoutError = MemberManager::NameInfos.size() == FirstInitSize;

		PrintDbgMessage<bDoDebugPrinting>("{} --> NameInfos.size(): 0x{:X} -> 0x{:X}", __FUNCTION__, FirstInitSize, MemberManager::NameInfos.size());
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestNameTranslation()
	{
		MemberManager::InitMemberNameCollisions();

		auto TestName = [](CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			auto [Index, bWasAdded] = MemberManager::MemberNames.FindOrAdd(Member.GetValidName());

			if (bWasAdded)
			{
				PrintDbgMessage<bDoDebugPrinting>("Error on name '{}'. Name was not found in MemberNameTable!", Member.GetValidName());
				return false;
			}

			auto It = Container.begin();

			uint64 InContainerIndex = 0x0;
			uint64 TranslatedIndex = 0x0;

			while (It != Container.end())
			{
				It = std::find_if(It, Container.end(), [Index](const NameInfo& Value) -> bool { return Value.Name == Index; });
				if (It == Container.end())
				{
					PrintDbgMessage<bDoDebugPrinting>("Error on name '{}'. Name was not found in NameContainer!", Member.GetValidName());
					return false;
				}

				InContainerIndex = It - Container.begin();
				TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

				if (InContainerIndex == TranslatedIndex)
					return true;

				std::advance(It, 1);
			}

			PrintDbgMessage<bDoDebugPrinting>("Struct: {}\nMember: {}", Struct.GetFullName(), Member.GetName());
			PrintDbgMessage<bDoDebugPrinting>("Error on name '{}'. TranslationIndex != Index (0x{:X} <<  != 0x{:X})", Member.GetValidName(), TranslatedIndex, InContainerIndex);

			return true;
		};

		auto TestEvenHarder = [](CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			uint64 TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

			if (MemberManager::MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName() != Member.GetValidName())
			{
				PrintDbgMessage<bDoDebugPrinting>("Error '{}' != '{}'", MemberManager::MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName(), Member.GetValidName());
				return false;
			}

			return true;
		};

		bool bSuccededTestWithoutError = true;

		for (auto& [StructIdx, Container] : MemberManager::NameInfos)
		{
			UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

			for (UEProperty Property : Struct.GetProperties())
			{
				SetBoolIfFailed(bSuccededTestWithoutError, TestName(Container, Struct, Property));
				SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(Container, Struct, Property));
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				SetBoolIfFailed(bSuccededTestWithoutError, TestName(Container, Struct, Function));
				SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(Container, Struct, Function));
			
				if (!Function.HasMembers())
					continue;

				CollisionManager::NameContainer& FuncContainer = MemberManager::NameInfos[Function.GetIndex()];

				for (UEProperty Property : Function.GetProperties())
				{
					SetBoolIfFailed(bSuccededTestWithoutError, TestName(FuncContainer, Function, Property));
					SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(FuncContainer, Function, Property));
				}
			}
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestMemberIterator()
	{
		MemberManager::InitMemberNameCollisions();

		PredefinedMemberLookupMapType PredefinedMembers;

		UEClass ULevel = ObjectArray::FindClassFast("Level");
		const int32 ULevelIndex = ULevel.GetIndex();

		std::vector<PredefinedMember>& ULevelMembers = PredefinedMembers[ULevelIndex].Members;

		ULevelMembers =
		{
			PredefinedMember{ "void*", "StaticPtr", 0x00, 0x08, 0x01, 0x08, true, false, 0xFF },
			PredefinedMember{ "TArray<class AActor*>", "Actors", Off::ULevel::Actors, 0x10, 0x01, 0x08, false ,false, 0xFF },
			PredefinedMember{ "std::nullptr_t", "MadeUpShit", 0x140, 0x08, 0x03, 0x08, false ,false, 0xFF },
			PredefinedMember{ "uint8", "bIsBitfield", 0x30, 0x01, 0x01, 0x01, false, true, 0x4 }
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
		MemberManager::InitMemberNameCollisions();

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
		APawnFunctions =
		{ 
			PredefinedFunction{ 
				"IsReelPawn", 
				"bool IsReelPawn(bool bIsGuaranteedToBePawn)", 
				"bool APawn::IsReelPawn(bool bIsGuaranteedToBePawn)",
				"{\n\treturn bIsGuaranteedToBePawn + 4;\n}"
				, false, false
			},
			PredefinedFunction{
				"GetActors",
				"TArray<class AActor*>* GetActors(int a1)",
				"TArray<class AActor*>* APawn::GetActors(int a1)",
				"{\n\treturn reinterpret_cast<TArray<class AActor*>*>(&a2);\n}"
				, true, false
			},
			PredefinedFunction{
				"GetNullPtr",
				"std::nullptr_t GetNullPtr()",
				"",
				"{\n\treturn nullptr;\n}"
				, false, true
			},
			PredefinedFunction{
				"StaticTestFunc",
				"void StaticTestFunc(bool* bIsGuaranteedToBePawn)",
				"",
				"{\n\t*bIsGuaranteedToBePawn = true;\n}"
				, true, true
			},
		};

		std::sort(APawnFunctions.begin(), APawnFunctions.end(), ComparePredefinedFunctions);

		StructManager::Init();
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