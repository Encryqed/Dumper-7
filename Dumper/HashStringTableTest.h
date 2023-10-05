#pragma once
#include "HashStringTable.h"
#include "ObjectArray.h"

#include <iostream>

class HashStringTableTest
{
public:
	static inline void TestAll()
	{
		TestReallocation();
		TestUniqueNames();
		std::cout << std::endl;
	}

	static inline void TestReallocation()
	{
		HashStringTable Desktop(0x5000);

		bool bIsEverythingFine = true;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				//int32 RawNameSize = 0x0;
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
		HashStringTable Desktop(0x5000);

		std::vector<std::string> UniqueNames;
		std::vector<std::string> RandomNamesToDuplicate;

		for (auto Obj : ObjectArray())
		{
			if (Obj.IsA(EClassCastFlags::Struct))
			{
				std::string UniqueName = Obj.GetCppName();

				if (Desktop.FindOrAdd(UniqueName).second)
					UniqueNames.push_back(UniqueName);

				if (Desktop.FindOrAdd(UniqueName).second)
					std::cout << "Added duplicate: \"" << UniqueName << "\"\n";

				if (rand() % 4 == 0)
					RandomNamesToDuplicate.push_back(UniqueName);
			}
		}

		for (auto& Dupliate : RandomNamesToDuplicate)
		{
			if (Desktop.FindOrAdd(Dupliate).second)
				std::cout << "Added duplicate: \"" << Dupliate << "\"\n";
		}

		for (const StringEntry& Entry : Desktop)
		{
			if (Entry.GetUniqueName() == "FSetToggle")
				std::cout << std::endl; // break;

			if (Entry.GetFullName().length() >= 1024)
				std::cout << std::endl; // break;


			std::cout << "Hash: " << std::dec << +Entry.GetHash() << "\n";
			std::cout << "Entry: " << Entry.GetUniqueName() << std::endl;
		}
	}
};