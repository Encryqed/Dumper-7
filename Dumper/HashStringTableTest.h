#pragma once
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "TestBase.h"

#include <iostream>

class HashStringTableTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestReallocation<bDoDebugPrinting>();
		TestUniqueNames<bDoDebugPrinting>();
		TestUniqueMemberNames<bDoDebugPrinting>();
		TestUniqueStructNames<bDoDebugPrinting>();
		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestReallocation()
	{
		HashStringTable Desktop;

		uint64 SizeCounter = 0x0;
		uint64 NameCounter = 0x0;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;
			
			std::string FullName = Obj.GetFullName();
			
			if (Desktop.FindOrAdd(FullName).second)
			{
				SizeCounter += FullName.size();
				NameCounter++;
			}
		}

		bool bSuccededTestWithoutError = (SizeCounter + (NameCounter * StringEntry::StringEntrySizeWithoutStr)) == Desktop.GetTotalUsedSize();

		if (!bSuccededTestWithoutError)
			PrintDbgMessage<bDoDebugPrinting>("Size 0x{:X} != 0x{:X}", SizeCounter, Desktop.GetTotalUsedSize());

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUniqueNames()
	{
		HashStringTable Desktop;

		std::vector<std::string> UniqueNames;
		std::vector<std::string> RandomNamesToDuplicate;

		bool bSuccededTestWithoutError = true;

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
				{
					PrintDbgMessage<bDoDebugPrinting>("Added duplicate: '{}'", UniqueName);
					SetBoolIfFailed(bSuccededTestWithoutError, false);

				}

				if (rand() % 8 == 0)
					RandomNamesToDuplicate.push_back(UniqueName);
			}
		}

		for (auto& Dupliate : RandomNamesToDuplicate)
		{
			if (Desktop.FindOrAdd(Dupliate).second)
			{
				PrintDbgMessage<bDoDebugPrinting>("Added duplicate2: '{}'", Dupliate);
				SetBoolIfFailed(bSuccededTestWithoutError, false);
			}
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

		PrintDbgMessage<bDoDebugPrinting>("\nTotal iterationss: 0x{}", TotalIterations);
		PrintDbgMessage<bDoDebugPrinting>("Total string-comparisons: 0x{}", (TotalIterations - NumAvoidedStingComparisons));
		PrintDbgMessage<bDoDebugPrinting>("Percentage avoided: {:2.03f}", (NumAvoidedStingComparisons / static_cast<double>(TotalIterations)));

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUniqueMemberNames()
	{
		HashStringTable Desktop;

		bool bSuccededTestWithoutError = true;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			for (auto Property : Obj.Cast<UEStruct>().GetProperties())
			{
				Desktop.FindOrAdd(Property.GetValidName());
			}
		}

		if constexpr (bDoDebugPrinting)
		{
			Desktop.DebugPrintStats();
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestUniqueStructNames()
	{
		HashStringTable Desktop;

		bool bSuccededTestWithoutError = true;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			Desktop.FindOrAdd(Obj.GetCppName());
		}

		if constexpr (bDoDebugPrinting)
		{
			Desktop.DebugPrintStats();
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};