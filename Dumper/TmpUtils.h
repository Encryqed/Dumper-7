#pragma once
#include <string>
#include <algorithm>
#include <cassert>

namespace Utils
{
	/* Credits: https://en.cppreference.com/w/cpp/string/byte/tolower */
	inline std::string StrToLower(std::string String)
	{
		std::transform(String.begin(), String.end(), String.begin(), [](unsigned char C) { return std::tolower(C); });

		return String;
	}
}


namespace FileNameHelper
{
	inline void MakeValidFileName(std::string& InOutName)
	{
		for (char& c : InOutName)
		{
			if (c == '<' || c == '>' || c == ':' || c == '\"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
				c = '_';
		}
	}
}


template<typename T>
constexpr T Align(T Size, T Alignment)
{
	static_assert(std::is_integral_v<T>, "Align can only hanlde integral types!");
	assert(Alignment != 0 && "Alignment was 0, division by zero exception.");

	const T RequiredAlign = Alignment - (Size % Alignment);

	return Size + (RequiredAlign != Alignment ? RequiredAlign : 0x0);
}

template<typename CharType>
inline int32_t StrlenHelper(const CharType* Str)
{
	if constexpr (std::is_same<CharType, char>())
	{
		return strlen(Str);
	}
	else
	{
		return wcslen(Str);
	}
}

template<typename CharType>
inline bool StrnCmpHelper(const CharType* Left, const CharType* Right, size_t NumCharsToCompare)
{
	if constexpr (std::is_same<CharType, char>())
	{
		return strncmp(Left, Right, NumCharsToCompare) == 0;
	}
	else
	{
		return wcsncmp(Left, Right, NumCharsToCompare) == 0;
	}
}

