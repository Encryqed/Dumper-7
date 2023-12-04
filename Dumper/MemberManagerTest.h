#pragma once
#include <unordered_set>
#include "MemberManager.h"

class MemberManagerTest
{
public:
	static inline void TestAll()
	{
		TestInit();
		TestNameTranslationCollisions();
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

	static inline void TestNameTranslationCollisions()
	{
		std::unordered_set<uint64> Idx;
		Idx.reserve(0x5000);

		uint64 Counter = 0x0;

		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			UEStruct Struct = Obj.Cast<UEStruct>();

			for (UEProperty Property : Struct.GetProperties())
			{
				uint64 Key = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Property);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					std::cout << "Property-Duplication, Count = " << Counter << ", Key = " << Key << std::endl;
				}
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				uint64 Key = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Function);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					std::cout << "Function-Duplication, Count = " << Counter++ << ", Key = " << Key << std::endl;
				}

				for (UEProperty Property : Function.GetProperties())
				{
					uint64 Key2 = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Property);

					Counter++;
					if (!Idx.insert(Key2).second)
					{
						std::cout << "Property-Duplication, Count = " << Counter++ << ", Key = " << Key2 << std::endl;
					}
				}
			}
		}

		std::cout << __FUNCTION__ << " --> NO FAILURE!\n" << std::endl;
	}

	static inline void TestNameTranslation()
	{
		MemberManager::InitMemberNameCollisions();

		auto TestName = [](NameContainer& Container, auto Member) -> bool
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

			int32 InContainerIndex = It - Container.begin();
			int32 TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Member));

			if (InContainerIndex != TranslatedIndex)
			{
				std::cout << "Error on name '" << Member.GetValidName() << "'. TranslationIndex != Index (" << TranslatedIndex << " != " << InContainerIndex << ")" << std::endl;
				return false;
			}

			return true;
		};

		auto TestEvenHarder = [](NameContainer& Container, auto Member) -> bool
		{
			int32 TranslatedIndex = MemberManager::TranslationMap.at(CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Member));

			if (MemberManager::MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName() != Member.GetValidName())
			{
				std::cout << "Error on name '" << Member.GetValidName() << "'. Index doesn't equal name!" << std::endl;
				return false;
			}

			return true;
		};

		for (auto& [StructIdx, Container] : MemberManager::NameInfos)
		{
			UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

			for (UEProperty Property : Struct.GetProperties())
			{
				TestName(Container, Property);
				TestEvenHarder(Container, Property);
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				TestName(Container, Function);
				TestEvenHarder(Container, Function);

				NameContainer& FuncContainer = MemberManager::NameInfos[Function.GetIndex()];

				for (UEProperty Property : Struct.GetProperties())
				{
					TestName(FuncContainer, Property);
					TestEvenHarder(FuncContainer, Property);
				}
			}
		}

		std::cout << __FUNCTION__ << " --> NO FAILURE!\n" << std::endl;
	}
};