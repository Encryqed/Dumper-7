#pragma once
#include "StringTable.h"

class StringTableTest
{
public:
	static void TestAll()
	{
		TestNames();
		TestUniqueNames();

		std::cout << std::endl;
	}

	static void TestNames()
	{
		StringTable Desk(0x50000);

		std::vector<std::string> Names;
		std::vector<std::string> FullNames;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				int32 RawNameSize = 0x0;
				std::string FullName = Obj.GetFullName(RawNameSize);

				if (FullName.find("IceDeimos") != -1)
				{
					std::cout << "FFullname: " << FullName << std::endl;
				}

				Names.push_back(Obj.GetName());
				FullNames.push_back(FullName);
				Desk.FindOrAdd(FullName.c_str(), (FullName.size() - RawNameSize), RawNameSize);
			}
		}

		bool bIsEverythingFine = true;

		int i = 0;
		for (const auto& Entry : Desk)
		{
			if (Entry.GetFullName() != FullNames[i])
			{
				bIsEverythingFine = false;
				std::cout << Entry.GetFullName() << " != " << FullNames[i] << std::endl;
			}

			if (Entry.GetRawName() != Names[i])
			{
				bIsEverythingFine = false;
				std::cout << Entry.GetRawName() << " != " << Names[i] << std::endl;
			}
			
			i++;
		}

		std::cout << __FUNCTION__ << ": " << (bIsEverythingFine ? "Everything is fine!" : "Nothing is fine!") << std::endl;
	}

	static void TestUniqueNames()
	{
		StringTable Desk(0x50000);
		Desk.SwitchToCheckedMode();

		std::vector<std::string> UniqueNames;
		std::vector<std::string> RandomNamesToDuplicate;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				std::string UniqueName = Obj.GetCppName();

				if (Desk.FindOrAdd(UniqueName).bAddedNew)
					UniqueNames.push_back(UniqueName);

				if (Desk.FindOrAdd(UniqueName).bAddedNew)
					std::cout << "Added duplicate: \"" << UniqueName << "\"\n";

				if (rand() % 4 == 0)
					RandomNamesToDuplicate.push_back(UniqueName);
			}
		}

		for (auto& Dupliate : RandomNamesToDuplicate)
		{
			if (Desk.FindOrAdd(Dupliate).bAddedNew)
				std::cout << "Added duplicate: \"" << Dupliate << "\"\n";
		}

		/*
		double AverageNumStrCompAvoidedByHash = 0.0;
		uint64_t TotalAvoidedStrComparisonsByHash = 0x0;
		uint64_t NumAvoidedStingComparisons = 0x0;
		uint64_t TotalIterations = 0x0;
		uint64_t NumTotalHashComparisons = 0x0;

		for (auto& UniqueName : UniqueNames)
		{
			uint8 Hash = SmallPearsonHash(UniqueName.c_str());
			int32 Length = UniqueName.size();

			int32 CurrNumStrCompAvoidedByHash = 0x0;

			for (auto It = StringTable::StringTableIterator(Desk, Desk.UncheckedSize); It != Desk.end(); ++It, TotalIterations++)
			{
				const StringEntry& Entry = *It;

				const bool bStringsAreEqual = Entry.GetUniqueName() != UniqueName;

				if (Entry.Length == Length)
					NumTotalHashComparisons++;

				if (Entry.Length != Length || Entry.Hash != Hash)
					NumAvoidedStingComparisons++;

				if (Entry.Length == Length && Entry.Hash != Hash)
				{
					if (bStringsAreEqual)
						CurrNumStrCompAvoidedByHash++;
				}

				//if (Entry.Hash == Hash && Entry.Length == Length)
				//{
				//	//if (Entry.GetUniqueName() != UniqueName)
				//	//{
				//	//	std::cout << "Both has and length are equal: " << Entry.GetUniqueName() << " <---> " << UniqueName << "\n";
				//	//}
				//}

			}

			TotalAvoidedStrComparisonsByHash += CurrNumStrCompAvoidedByHash;
			AverageNumStrCompAvoidedByHash = (AverageNumStrCompAvoidedByHash + CurrNumStrCompAvoidedByHash) / 2.0;
		}

		std::cout << "\n" << std::endl;
		std::cout << "Total iterationss: 0x" << TotalIterations << std::endl; 
		std::cout << "Total string-comparisons: 0x" << (TotalIterations - NumAvoidedStingComparisons) << std::endl;
		std::cout << "Percentage avoided: " << (NumAvoidedStingComparisons / static_cast<double>(TotalIterations)) << std::endl;
		std::cout << "Total avoided comparisons due to hash: 0x" << TotalAvoidedStrComparisonsByHash << std::endl;
		std::cout << "Average avoided comparisons due to hash: " << AverageNumStrCompAvoidedByHash << std::endl;
		std::cout << "Percentage avoided by hash: " << (TotalAvoidedStrComparisonsByHash / static_cast<double>(TotalIterations)) << std::endl;
		std::cout << "Percentage of hash missmatches: " << (TotalAvoidedStrComparisonsByHash / static_cast<double>(NumTotalHashComparisons)) << std::endl;
		std::cout << "\n" << std::endl;
		*/

		//double AverageHashCollisions = 0.0;
		//double AverageLengthCollisions = 0.0;
		//
		//for (auto& UniqueName : UniqueNames)
		//{
		//	uint8 Hash = SmallPearsonHash(UniqueName.c_str());
		//	int32 Length = UniqueName.size();
		//
		//	double Iterations = 0x0;
		//	int32 HashCollisions = 0x0;
		//	int32 LengthCollisions = 0x0;
		//
		//	for (auto It = StringTable::StringTableIterator(Desk, Desk.UncheckedSize); It != Desk.end(); ++It, Iterations += 1.0)
		//	{
		//		const StringEntry& Entry = *It;
		//
		//		if (Entry.Hash == Hash)
		//			HashCollisions++;
		//
		//		if (Entry.Length == Length)
		//			LengthCollisions++;
		//	}
		//
		//	double CurrentAverageHashCollisions = HashCollisions / Iterations;
		//	double CurrentAverageLengthCollisions = LengthCollisions / Iterations;
		//
		//	AverageHashCollisions = (AverageHashCollisions + CurrentAverageHashCollisions) / 2.0;
		//	AverageLengthCollisions = (AverageLengthCollisions + CurrentAverageLengthCollisions) / 2.0;
		//}
		//
		//std::cout << "AverageHashCollisions: " << AverageHashCollisions << std::endl;
		//std::cout << "AverageLengthCollisions: " << AverageLengthCollisions << std::endl;


		bool bIsEverythingFine = true;

		int i = 0;
		for (const auto& Entry : Desk)
		{
			if (Entry.GetUniqueName() != UniqueNames[i])
			{
				bIsEverythingFine = false;
				std::cout << Entry.GetUniqueName() << " != " << UniqueNames[i] << std::endl;
			}

			i++;
		}

		std::cout << __FUNCTION__ << ": " << (bIsEverythingFine ? "Everything is fine!" : "Nothing is fine!") << std::endl;
	}
};