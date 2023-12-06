#pragma once
#include <unordered_set>
#include "MemberManager.h"

class MemberManagerTest
{
public:
	static inline void TestAll()
	{
		TestInit();
		TestNameTranslation();
		TestMemberIterator();

		std::cout << std::endl;
	}

	static inline void TestInit()
	{
		MemberManager::InitMemberNameCollisions();

		size_t Size = MemberManager::NameInfos.size();

		MemberManager::InitMemberNameCollisions();
		MemberManager::InitMemberNameCollisions();
		MemberManager::InitMemberNameCollisions();

		std::cout << __FUNCTION__ << " --> NameInfos.size(): 0x" << Size << " -> 0x" << std::hex << MemberManager::NameInfos.size() << "\n" << std::endl;
	}

	static inline void TestNameTranslation()
	{
		MemberManager::InitMemberNameCollisions();

		auto TestName = [](CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			auto [Index, bWasAdded] = MemberManager::MemberNames.FindOrAdd(Member.GetValidName());

			if (bWasAdded)
			{
				std::cout << "Error on name '" << Member.GetValidName() << "'. Name was not found in MemberNameTable!" << std::endl;
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
					std::cout << "Error on name '" << Member.GetValidName() << "'. Name was not found in NameContainer!" << std::endl;
					return false;
				}

				InContainerIndex = It - Container.begin();
				TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

				if (InContainerIndex == TranslatedIndex)
					return true;

				std::advance(It, 1);
			}

			std::cout << "Struct: " << Struct.GetFullName() << std::endl;
			std::cout << "Member: " << Member.GetName() << std::endl;

			std::cout << "Error on name '" << Member.GetValidName() << "'. TranslationIndex != Index (" << TranslatedIndex << " != " << InContainerIndex << ")" << std::endl;

			return true;
		};

		auto TestEvenHarder = [](CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			uint64 TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

			if (MemberManager::MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName() != Member.GetValidName())
			{
				std::cout << "Error: '" << MemberManager::MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName() << "' != '" << Member.GetValidName() << "'" << std::endl;
				return false;
			}

			return true;
		};

		for (auto& [StructIdx, Container] : MemberManager::NameInfos)
		{
			UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

			for (UEProperty Property : Struct.GetProperties())
			{
				TestName(Container, Struct, Property);
				TestEvenHarder(Container, Struct, Property);
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				TestName(Container, Struct, Function);
				TestEvenHarder(Container, Struct, Function);
			
				if (!Function.HasMembers())
					continue;

				CollisionManager::NameContainer& FuncContainer = MemberManager::NameInfos[Function.GetIndex()];

				for (UEProperty Property : Function.GetProperties())
				{
					TestName(FuncContainer, Function, Property);
					TestEvenHarder(FuncContainer, Function, Property);
				}
			}
		}

		std::cout << __FUNCTION__ << " --> NO FAILURE!\n" << std::endl;
	}

	static inline void TestMemberIterator()
	{
		MemberManager::InitMemberNameCollisions();

		PredefinedMemberLookupMapType PredefinedMembers;

		UEClass ULevel = ObjectArray::FindClassFast("Level");
		const int32 ULevelIndex = ULevel.GetIndex();

		static auto PredefMemberLessThanCompare = [](const PredefinedMember& Left, const PredefinedMember& Right) -> bool
		{
			if (Left.bIsStatic && Right.bIsStatic)
				return Left.Name < Right.Name;

			if (Left.bIsStatic || Right.bIsStatic)
				return Left.bIsStatic > Right.bIsStatic;
			
			return Left.Offset < Right.Offset;
		};

		std::vector<PredefinedMember>& ULevelMembers = PredefinedMembers[ULevelIndex].Members;

		ULevelMembers =
		{
			PredefinedMember{ "void*", "StaticPtr", 0x00, 0x08, 0x01, 0x08, true, false, 0xFF },
			PredefinedMember{ "TArray<class AActor*>", "Actors", Off::ULevel::Actors, 0x10, 0x01, 0x08, false ,false, 0xFF },
			PredefinedMember{ "std::nullptr_t", "MadeUpShit", 0x140, 0x08, 0x03, 0x08, false ,false, 0xFF },
			PredefinedMember{ "uint8", "bIsBitfield", 0x30, 0x01, 0x01, 0x01, false, true, 0x4 }
		};

		std::sort(ULevelMembers.begin(), ULevelMembers.end(), PredefMemberLessThanCompare);

		StructManager::Init();
		MemberManager::SetPredefinedMemberLookup(&PredefinedMembers);

		MemberManager Members(ULevel);

		for (PropertyWrapper Wrapper : Members.IterateMembers())
		{
			std::cout << (Wrapper.IsUnrealProperty() ? Wrapper.GetUnrealProperty().GetCppType() : Wrapper.GetType()) << " " << Wrapper.GetName()  << ";" << std::endl;
		}
	}
};