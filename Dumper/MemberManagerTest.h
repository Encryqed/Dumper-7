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

			auto It = std::find_if(Container.begin(), Container.end(), [Index](const NameInfo& Value) -> bool { return Value.Name == Index; });
			if (It == Container.end())
			{
				std::cout << "Error on name '" << Member.GetValidName() << "'. Name was not found in NameContainer!" << std::endl;
				return false;
			}

			uint64 InContainerIndex = It - Container.begin();
			uint64 TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

			if (InContainerIndex != TranslatedIndex)
			{
				std::cout << "Error on name '" << Member.GetValidName() << "'. TranslationIndex != Index (" << TranslatedIndex << " != " << InContainerIndex << ")" << std::endl;
				return false;
			}

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
};