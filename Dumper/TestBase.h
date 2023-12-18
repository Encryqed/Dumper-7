#pragma once
#include <iostream>
#include <format>

class TestBase
{
protected:
	template<bool bPrint, bool bFlushWithEndl = true, typename... FmtArgs>
	static inline void PrintDbgMessage(const char* FmtStr, FmtArgs&&... Args)
	{
		if constexpr (bPrint)
		{
			std::cout << std::vformat(FmtStr, std::make_format_args(Args...));

			if constexpr (bFlushWithEndl)
				std::cout << std::endl;
		}
	}

	static inline void SetBoolIfFailed(bool& BoolToSet, bool bSucceded)
	{
		BoolToSet = BoolToSet && bSucceded;
	}
};