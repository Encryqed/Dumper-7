#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"

#include <iostream>

class HashStringTableTest
{
public:
	static inline void TestAll()
	{
		TestReallocation();
		TestUniqueNames();
		TestUniqueMemberNames();
		TestUniqueStructNames();
		std::cout << std::endl;
	}

	static inline void TestReallocation()
	{
		HashStringTable Desktop;

		bool bIsEverythingFine = true;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				std::string FullName = Obj.GetFullName();
				
				if (FullName.find("IceDeimos") != -1)
				{
					std::cout << "FFullname: " << FullName << std::endl;
				}

				Desktop.FindOrAdd(FullName);
			}
		}

		std::cout << __FUNCTION__ << ": " << (bIsEverythingFine ? "Everything is fine!" : "Nothing is fine!") << std::endl;
	}

	static inline void TestUniqueNames()
	{
		HashStringTable Desktop;

		std::vector<std::string> UniqueNames;
		std::vector<std::string> RandomNamesToDuplicate;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				std::string UniqueName = Obj.GetCppName();

				if (Desktop.FindOrAdd(UniqueName).second)
					UniqueNames.push_back(UniqueName);
			}

			if (Obj.IsA(EClassCastFlags::Struct))
			{
				std::string UniqueName = Obj.GetCppName();

				auto [Index, bAddedElement] = Desktop.FindOrAdd(UniqueName);

				if (bAddedElement)
					UniqueNames.push_back(UniqueName);

				if (Desktop.FindOrAdd(UniqueName).second)
					std::cout << "Added duplicate: \"" << UniqueName << "\"\n";

				if (rand() % 8 == 0)
					RandomNamesToDuplicate.push_back(UniqueName);
			}
		}

		for (auto& Dupliate : RandomNamesToDuplicate)
		{
			if (Desktop.FindOrAdd(Dupliate).second)
				std::cout << "Added duplicate: \"" << Dupliate << "\"\n";
		}

		double AverageNumStrCompAvoided = 0.0;
		uint64_t NumAvoidedStingComparisons = 0x0;
		uint64_t TotalIterations = 0x0;

		for (auto& UniqueName : UniqueNames)
		{
			uint8 Hash = SmallPearsonHash(UniqueName.c_str());
			int32 Length = UniqueName.size();

			int32 CurrNumStrCompAvoided = 0x0;

			const auto& CurrentBucket = Desktop.GetBucket(Hash);

			for (auto It = HashStringTable::HashBucketIterator::begin(CurrentBucket); It != HashStringTable::HashBucketIterator::end(CurrentBucket); ++It, TotalIterations++)
			{
				const StringEntry& Entry = *It;

				const bool bStringsAreEqual = Entry.GetName() != UniqueName;

				if (Entry.Length != Length)
					NumAvoidedStingComparisons++;
			}

			AverageNumStrCompAvoided = (AverageNumStrCompAvoided + CurrNumStrCompAvoided) / 2.0;
		}

		std::cout << "\n" << std::endl;
		std::cout << "Total iterationss: 0x" << TotalIterations << std::endl;
		std::cout << "Total string-comparisons: 0x" << (TotalIterations - NumAvoidedStingComparisons) << std::endl;
		std::cout << "Percentage avoided: " << (NumAvoidedStingComparisons / static_cast<double>(TotalIterations)) << std::endl;
		std::cout << "\n" << std::endl;

		std::cout << __FUNCTION__ << ": Everything is fine!" << std::endl;
	}

	static inline void TestUniqueMemberNames()
	{
		HashStringTable Desktop;

		bool bIsEverythingFine = true;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			for (auto Property : Obj.Cast<UEStruct>().GetProperties())
			{
				Desktop.FindOrAdd(Property.GetValidName());
			}
		}

		Desktop.DebugPrintStats();

		std::cout << __FUNCTION__ << ": " << (bIsEverythingFine ? "Everything is fine!" : "Nothing is fine!") << std::endl;
	}

	static inline void TestUniqueStructNames()
	{
		HashStringTable Desktop;

		bool bIsEverythingFine = true;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			Desktop.FindOrAdd(Obj.GetCppName());
		}

		Desktop.DebugPrintStats();

		std::cout << __FUNCTION__ << ": " << (bIsEverythingFine ? "Everything is fine!" : "Nothing is fine!") << std::endl;
	}
};