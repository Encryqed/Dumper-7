#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "NameCollisionHandler.h"
#include "TestBase.h"

inline uint64 TempGlobalNameCounter = 0x0;


class CollisionManagerTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestKeyCreationFunctions<bDoDebugPrinting>();
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
				uint64 Key = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Property);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					PrintDbgMessage<bDoDebugPrinting>("Property-Duplication, Count = {}, Key = {}", Counter, Key);
					SetBoolIfFailed(bSuccededTestWithoutError, false);
				}
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				uint64 Key = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Function);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					PrintDbgMessage<bDoDebugPrinting>("Function-Duplication, Count = {}, Key = {}", Counter, Key);
					SetBoolIfFailed(bSuccededTestWithoutError, false);
				}

				for (UEProperty Property : Function.GetProperties())
				{
					uint64 Key2 = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Property);

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
	static inline void TestMakeNamesUnique()
	{
		HashStringTable MemberNames;

		CollisionManager::NameInfoMapType Names;
		CollisionManager::TranslationMapType NameTranslations;

		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			CollisionManager::AddStructToNameContainer(Names, NameTranslations, MemberNames, Obj.Cast<UEStruct>());
		}

		uint64 NumNames = 0x0;
		uint64 NumCollidingNames = 0x0;

		bool bSuccededTestWithoutError = true;

		for (auto [StructIdx, NameContainer] : Names)
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

		PrintCollidingNames(Names, MemberNames);


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
