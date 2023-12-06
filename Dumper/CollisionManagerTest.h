#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "NameCollisionHandler.h"

inline uint64 TempGlobalNameCounter = 0x0;

inline void PrintCollidingNames(const CollisionManager::NameInfoMapType& Names, const HashStringTable& MemberNames)
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
					std::cout << "SMCnt-";
				}
				if (NameInfo.MemberNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.MemberNameCollisionCount - 1));
					std::cout << "MbmCnt-";
				}
			}
			else if (OwnCollisionType == ECollisionType::FunctionName)
			{
				if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0)
				{
					Name = ("Func_" + Name);
					std::cout << "Fnc-";
				}
				if (NameInfo.FunctionNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.FunctionNameCollisionCount - 1));
					std::cout << "FncCnt-";
				}
			}
			else if (OwnCollisionType == ECollisionType::ParameterName)
			{
				if (NameInfo.MemberNameCollisionCount > 0x0 || NameInfo.SuperMemberNameCollisionCount > 0x0 || NameInfo.FunctionNameCollisionCount > 0x0 || NameInfo.SuperFuncNameCollisionCount > 0x0)
				{
					Name = ("Param_" + Name);
					std::cout << "Prm-";
				}
				if (NameInfo.ParamNameCollisionCount > 0x0)
				{
					Name += ("_" + std::to_string(NameInfo.ParamNameCollisionCount - 1));
					std::cout << "PrmCnt-";
				}
			}

			std::cout << "Name: " << Name << "\n";
		}
	}

	std::cout << std::endl;
}

class CollisionManagerTest
{
public:
	static inline void TestAll()
	{
		TestKeyCreationFunctions();
		TestMakeNamesUnique();

		std::cout << std::endl;
	}

	static inline void TestKeyCreationFunctions()
	{
		std::unordered_set<uint64> Idx;
		Idx.reserve(0x1000);

		uint64 Counter = 0x0;

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
					std::cout << "Property-Duplication, Count = " << Counter << ", Key = " << Key << std::endl;
				}
			}

			for (UEFunction Function : Struct.GetFunctions())
			{
				uint64 Key = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Function);

				Counter++;
				if (!Idx.insert(Key).second)
				{
					std::cout << "Function-Duplication, Count = " << Counter++ << ", Key = " << Key << std::endl;
				}

				for (UEProperty Property : Function.GetProperties())
				{
					uint64 Key2 = CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Property);

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

		for (auto [StructIdx, NameContainer] : Names)
		{
			const UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(StructIdx);
			const bool bIsFunction = Struct.IsA(EClassCastFlags::Function);

			NumNames += NameContainer.size();

			for (auto NameInfo : NameContainer)
			{
				if (!bIsFunction && static_cast<ECollisionType>(NameInfo.OwnType) == ECollisionType::ParameterName)
					std::cout << std::format("Invalid: Param in '{}'\n", Struct.GetName());

				if (NameInfo.HasCollisions())
					NumCollidingNames++;
			}
		}

		PrintCollidingNames(Names, MemberNames);

		std::cout << "GlobalNumNames: " << TempGlobalNameCounter << "\n" << std::endl;

		std::cout << "NumNames: " << NumNames << std::endl;
		std::cout << "NumCollidingNames: " << NumCollidingNames << std::endl;

		std::cout << std::format("Collisions in '%': {:.04f}%\n", static_cast<double>(NumCollidingNames) / NumNames) << std::endl;
	}
};
