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
				std::string FullName = Obj.GetFullName();

				Names.push_back(Obj.GetName());
				FullNames.push_back(FullName);
				Desk.Add(FullName);

				if (FullName.find("IceDeimos") != -1)
				{
					std::cout << "FFullname: " << FullName << std::endl;
				}
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

				if (Desk.Add(UniqueName) != -1)
					UniqueNames.push_back(UniqueName);
				
				if (Desk.Add(UniqueName) != -1)
					std::cout << "Added duplicate: \"" << UniqueName << "\"\n";

				if (rand() % 4 == 0)
					RandomNamesToDuplicate.push_back(UniqueName);
			}
		}

		for (auto& Dupliate : RandomNamesToDuplicate)
		{
			if (Desk.Add(Dupliate) != -1)
				std::cout << "Added duplicate: \"" << Dupliate << "\"\n";
		}

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