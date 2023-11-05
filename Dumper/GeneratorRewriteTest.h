#pragma once
#include "GeneratorRewrite.h"
#include "HashStringTable.h"
#include <cassert>

#define CHECK_RESULT(Result, Expected) \
if (Result != Expected) \
{ \
	std::cout << __FUNCTION__ << ":\nResult: \n" << Result << "\nExpected: \n" << Expected << std::endl; \
} \
else \
{ \
	std::cout << __FUNCTION__ << ": Everything is fine!" << std::endl; \
}

class GeneratorRewriteTest
{
public:
	static inline void TestAll()
	{
		TestGetherPackages();
		std::cout << std::endl;
	}

	static inline void TestGetherPackages()
	{
		auto Result = GeneratorRewrite::GatherPackages();

		for (auto& [Package, Info] : Result)
		{
			if (ObjectArray::GetByIndex(Package).GetName() == "Engine")
				std::cout << std::format("{}: {{\n\tNumPackageDependencies = 0x{:X},\n\tNumClasses = 0x{:X},\n\tNumStructs = 0x{:X},\n\tNumEnums = 0x{:X},\n\tNumFunctions = 0x{:X}\n}}\n"
				, ObjectArray::GetByIndex(Package).GetName()
				, Info.PackageDependencies.size()
				, Info.Classes.GetNumEntries()
				, Info.Structs.GetNumEntries()
				, Info.Enums.size()
				, Info.Functions.size());
		}
	}
};