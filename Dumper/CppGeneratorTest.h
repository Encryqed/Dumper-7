#pragma once
#include "CppGenerator.h"
#include <cassert>

class CppGeneratorTest
{
	bool TestMakeMember()
	{
		std::string Result = CppGenerator::MakeMemberString("uint32", "ThisIsNumber", "Very number, yes. (flags, and stuff)");
		std::string Expected = "	uint32                                   ThisIsNumber;                                          // Very number, yes. (flags, and stuff)";

		if (Result != Expected)
		{
			std::cout << "Result: \n" << Result << "\nExpected: \n" << Expected << std::endl;
		}
		else
		{
			std::cout << "Everything is fine!" << std::endl;
		}
	}
};