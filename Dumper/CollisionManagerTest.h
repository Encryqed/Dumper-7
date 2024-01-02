#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "CollisionManager.h"
#include "TestBase.h"

inline uint64 TempGlobalNameCounter = 0x0;


class CollisionManagerTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestKeyCreationFunctions<bDoDebugPrinting>();
		TestNameTranslation<bDoDebugPrinting>();
		TestMakeNamesUnique<bDoDebugPrinting>();

		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestKeyCreationFunctions()
	{
		std::unordered_set<uint64> Idx;
		Idx.reserve(0x1000);

		uint64 Counter = 0x0;

		bool bSuccededTestWithoutError = true;

		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			UEStruct Struct = Obj.Cast<UEStruct>();

			for (UEProperty Property : Struct.GetProperties())
			{
				uint64 Key = KeyFunctions::GetKeyForCollisionInfo(Struct, Property);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					PrintDbgMessage<bDoDebugPrinting>("Property-Duplication, Count = {}, Key = {}", Counter, Key);
					SetBoolIfFailed(bSuccededTestWithoutError, false);
				}
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				uint64 Key = KeyFunctions::GetKeyForCollisionInfo(Struct, Function);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					PrintDbgMessage<bDoDebugPrinting>("Function-Duplication, Count = {}, Key = {}", Counter, Key);
					SetBoolIfFailed(bSuccededTestWithoutError, false);
				}

				for (UEProperty Property : Function.GetProperties())
				{
					uint64 Key2 = KeyFunctions::GetKeyForCollisionInfo(Struct, Property);

					Counter++;
					if (!Idx.insert(Key2).second)
					{
						PrintDbgMessage<bDoDebugPrinting>("Property-Duplication, Count = {}, Key = {}", Counter, Key);
						SetBoolIfFailed(bSuccededTestWithoutError, false);
					}
				}
			}
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestNameTranslation()
	{
		MemberManager::Init();

		auto TestName = [](CollisionManager& Manager, const CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			auto [Index, bWasAdded] = Manager.MemberNames.FindOrAdd(Member.GetValidName());

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
				TranslatedIndex = Manager.TranslationMap.at(KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

				if (InContainerIndex == TranslatedIndex)
					return true;

				std::advance(It, 1);
			}

			PrintDbgMessage<bDoDebugPrinting>("Struct: {}\nMember: {}", Struct.GetFullName(), Member.GetName());
			PrintDbgMessage<bDoDebugPrinting>("Error on name '{}'. TranslationIndex != Index (0x{:X} <<  != 0x{:X})", Member.GetValidName(), TranslatedIndex, InContainerIndex);

			return true;
		};

		auto TestEvenHarder = [](CollisionManager& Manager, const CollisionManager::NameContainer& Container, UEStruct Struct, auto Member) -> bool
		{
			uint64 TranslatedIndex = Manager.TranslationMap.at(KeyFunctions::GetKeyForCollisionInfo(Struct, Member));

			if (Manager.MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName() != Member.GetValidName())
			{
				PrintDbgMessage<bDoDebugPrinting>("Error '{}' != '{}'", Manager.MemberNames.GetStringEntry(Container[TranslatedIndex].Name).GetName(), Member.GetValidName());
				return false;
			}

			return true;
		};

		bool bSuccededTestWithoutError = true;

		CollisionManager& Manager = MemberManager::MemberNames;

		for (const auto& [StructIdx, Container] : Manager.NameInfos)
		{
			UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

			for (UEProperty Property : Struct.GetProperties())
			{
				SetBoolIfFailed(bSuccededTestWithoutError, TestName(Manager, Container, Struct, Property));
				SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(Manager, Container, Struct, Property));
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				SetBoolIfFailed(bSuccededTestWithoutError, TestName(Manager, Container, Struct, Function));
				SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(Manager, Container, Struct, Function));

				if (!Function.HasMembers())
					continue;

				const CollisionManager::NameContainer& FuncContainer = Manager.NameInfos.at(Function.GetIndex());

				for (UEProperty Property : Function.GetProperties())
				{
					SetBoolIfFailed(bSuccededTestWithoutError, TestName(Manager, FuncContainer, Function, Property));
					SetBoolIfFailed(bSuccededTestWithoutError, TestEvenHarder(Manager, FuncContainer, Function, Property));
				}
			}
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestMakeNamesUnique()
	{
		CollisionManager Manager;

		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			Manager.AddStructToNameContainer(Obj.Cast<UEStruct>());
		}

		uint64 NumNames = 0x0;
		uint64 NumCollidingNames = 0x0;

		bool bSuccededTestWithoutError = true;

		for (auto [StructIdx, NameContainer] : Manager.NameInfos)
		{
			const UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);
			const bool bIsFunction = Struct.IsA(EClassCastFlags::Function);

			NumNames += NameContainer.size();

			for (auto NameInfo : NameContainer)
			{
				if (!bIsFunction && static_cast<ECollisionType>(NameInfo.OwnType) == ECollisionType::ParameterName)
				{
					PrintDbgMessage<bDoDebugPrinting>("Invalid: Param in '{}'\n", Struct.GetName());
					SetBoolIfFailed(bSuccededTestWithoutError, false);
				}

				if (NameInfo.HasCollisions())
					NumCollidingNames++;
			}
		}

		PrintCollidingNames(Manager.NameInfos, Manager.MemberNames);


		PrintDbgMessage<bDoDebugPrinting>("GlobalNumNames: {}", TempGlobalNameCounter);
		PrintDbgMessage<bDoDebugPrinting>("NumNames: {}", NumNames);
		PrintDbgMessage<bDoDebugPrinting>("NumCollidingNames: {}", NumCollidingNames);

		PrintDbgMessage<bDoDebugPrinting>("Collisions in '%': {:.04f}%", static_cast<double>(NumCollidingNames) / NumNames);

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

private:

	template<bool bDoDebugPrinting = false>
	static inline void PrintCollidingNames(const CollisionManager::NameInfoMapType& Names, const HashStringTable& MemberNames)
	{
		for (auto [StructIdx, NameContainer] : Names)
		{
			UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);

			for (auto NameInfo : NameContainer)
			{
				if (!NameInfo.HasCollisions())
					continue;

				ECollisionType OwnCollisionType = static_cast<ECollisionType>(NameInfo.OwnType);

				std::string Name = MemberNames.GetStringEntry(NameInfo.Name).GetName();

				//std::cout << "Nm: " << Name << "\n";

				// Order of sub-if-statements matters
				if (OwnCollisionType == ECollisionType::MemberName)
				{
					if (NameInfo.SuperMemberNameCollisionCount > 0x0)
					{
						Name += ("_" + Struct.GetValidName());
						PrintDbgMessage<bDoDebugPrinting, false>("SMCnt-");
					}
					if (NameInfo.MemberNameCollisionCount > 0x0)
					{
						Name += ("_" + std::to_string(NameInfo.MemberNameCollisionCount - 1));
						PrintDbgMessage<bDoDebugPrinting, false>("MbmCnt-");
					}
				}
				else if (OwnCollisionType == ECollisionType::FunctionName)
				{
					if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0)
					{
						Name = ("Func_" + Name);
						PrintDbgMessage<bDoDebugPrinting, false>("Fnc-");
					}
					if (NameInfo.FunctionNameCollisionCount > 0x0)
					{
						Name += ("_" + std::to_string(NameInfo.FunctionNameCollisionCount - 1));
						PrintDbgMessage<bDoDebugPrinting, false>("FncCnt-");
					}
				}
				else if (OwnCollisionType == ECollisionType::ParameterName)
				{
					if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0 || NameInfo.FunctionNameCollisionCount > 0x0 || NameInfo.SuperFuncNameCollisionCount > 0x0)
					{
						Name = ("Param_" + Name);
						PrintDbgMessage<bDoDebugPrinting, false>("Prm-");
					}
					if (NameInfo.ParamNameCollisionCount > 0x0)
					{
						Name += ("_" + std::to_string(NameInfo.ParamNameCollisionCount - 1));
						PrintDbgMessage<bDoDebugPrinting, false>("PrmCnt-");
					}
				}

				PrintDbgMessage<bDoDebugPrinting>("Name: {}", Name);
			}
		}

		PrintDbgMessage<bDoDebugPrinting>("");
	}
};
