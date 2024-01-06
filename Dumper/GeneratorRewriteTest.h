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
		std::cout << std::endl;
	}
};