#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

static bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			b = true;

		return b;
	}

	return true;
};

struct MemAddress
{
public:
	uint8* Address;

private:
	//pasted
	static std::vector<int32_t> PatternToBytes(const char* pattern)
	{
		auto bytes = std::vector<int>{};
		const auto start = const_cast<char*>(pattern);
		const auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current)
		{
			if (*current == '?')
			{
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else { bytes.push_back(strtoul(current, &current, 16)); }
		}
		return bytes;
	}
	static bool IsFunctionRet(uint8* Address)
	{
		int Align = 0x10 - (uintptr_t(Address) % 0x10);
		//if (Opcode == RET && (OpcodeBefore is a POP opcode || OpcodeTwoBefore is a different POP Opcode)
		return Address[0] == 0xC3 && Address[Align] == 0x40 && ((Address[-1] >= 0x58 && Address[-1] <= 0x5F) || (Address[-2] == 0x41 && (Address[-1] >= 0x58 && Address[-1] <= 0x5F)));
	}

public:
	inline MemAddress(void* Addr)
		: Address((uint8*)Addr)
	{
	}
	inline MemAddress(uintptr_t Addr)
		: Address((uint8*)Addr)
	{
	}

	explicit operator void*()
	{
		return Address;
	}
	explicit operator uintptr_t()
	{
		return uintptr_t(Address);
	}

	template<typename T = void>
	inline T* Get()
	{
		return Address;
	}

	MemAddress FindFunctionEnd()
	{
		if (!Address)
			return MemAddress(nullptr);

		int Align = 0x10 - (uintptr_t(Address) % 0x10);

		for (int i = 0; i < 0xFFFF; i++)
		{
			if (IsFunctionRet(Address + i))
			{
				return MemAddress(Address + i);
			}
			if ((uintptr_t(Address + i) % 0x10 == 0) && (Address[i] == 0x40 && (Address[i + 1] >= 0x50 && Address[i + 1] <= 0x57) && (Address[i + 2] >= 0x50 && Address[i + 2] <= 0x57)))
			{
				return MemAddress(Address + i);
			}
		}

		return  MemAddress(nullptr);
	}

	//Needs work
	//PlusMinus how many bytes forwards and backwards to search, if 0 it'll take function extend
	inline void* RelativePattern(const char* Pattern, int32_t PlusMinus = 0, int32_t Relative = 0)
	{
		if (!Address)
			return nullptr;

		std::vector<int32_t> Bytes = PatternToBytes(Pattern);
		const int32_t NumBytes = Bytes.size();

		const std::pair<int, int> Extend = { 0, 0 };
		const int32_t IterationCount = PlusMinus ? 2 * PlusMinus : Extend.second - Extend.first;
		uint8* StartAddress = Address - PlusMinus;

		for (auto i = 0ul; i < IterationCount; ++i)
		{
			bool found = true;
			for (auto j = 0ul; j < NumBytes; ++j)
			{
				if (StartAddress[i + j] != Bytes[j] && Bytes[j] != -1)
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				uintptr_t address = reinterpret_cast<uintptr_t>(&StartAddress[i]);
				if (Relative != 0)
				{
					address = ((address + Relative + 4) + *(int32_t*)(address + Relative));

					return (void*)address;
				}
				return (void*)address;
			}
		}
	}

	// every occurence of E8 counts as a call, use for max 1-5 calls only
	/* Negative index for search up, positive for search down  | goes beyond function-boundaries */
	inline void* GetCalledFunction(int32_t FunctionIndex)
	{
		if (!Address || FunctionIndex == 0)
			return nullptr;

		int32_t NumCalls = 0;

		for (int i = 0; i < (FunctionIndex > 0 ? 0xFFF : -0xFFF); (FunctionIndex > 0 ? i += 0x50 : i -= 0x50))
		{
			for (int j = 0; j < 0x50; j++)
			{
				//opcode: call
				if (Address[i + j] == 0xE8)
				{
					void* Func = (void*)(((0xFFFFFFFF00000000 | *(uint32*)(Address + i + j + 1)) + uintptr_t(Address + i + j)) + 5);

					if (++NumCalls == FunctionIndex)
					{
						return Func;
					}
				}
			}
		}

		return nullptr;
	}

	inline MemAddress FindNextFunctionStart()
	{
		if (!Address)
			return MemAddress(nullptr);

		uintptr_t FuncEnd = (uintptr_t)FindFunctionEnd();

		return FuncEnd % 0x10 != 0 ? FuncEnd + (0x10 - (FuncEnd % 0x10)) : FuncEnd;
	}
};

template<typename Type = const char*>
inline MemAddress FindByString(Type RefStr)
{
	uintptr_t ImageBase = uintptr_t(GetModuleHandle(0));
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeader);

	uint8_t* DataSection = nullptr;
	uint8_t* TextSection = nullptr;
	DWORD DataSize = 0;
	DWORD TextSize = 0;

	uint8_t* StringAddress = nullptr;

	for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

		std::string SectionName = (const char*)CurrentSection.Name;

		//std::cout << "Section: " << SectionName << " at 0x" << (void*)(CurrentSection.VirtualAddress + ImageBase) << "\n";

		if (SectionName == ".rdata" && !DataSection)
		{
			DataSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			DataSize = CurrentSection.Misc.VirtualSize;
		}
		else if (SectionName == ".text" && !TextSection)
		{
			TextSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			TextSize = CurrentSection.Misc.VirtualSize;
		}
	}

	for (int i = 0; i < DataSize; i++)
	{
		if (std::is_same<Type, const char*>())
		{
			if (strcmp((const char*)RefStr, (const char*)(DataSection + i)) == 0)
			{
				//std::cout << "FoundStr ref: " << (const char*)(DataSection + i) << "\n";

				StringAddress = DataSection + i;
			}
		}
		else
		{
			if (wcscmp((const wchar_t*)RefStr, (const wchar_t*)(DataSection + i)) == 0)
			{
				//std::wcout << L"FoundStr wref: " << (const wchar_t*)(DataSection + i) << L"\n";

				StringAddress = DataSection + i;
			}
		}
	}

	for (int i = 0; i < TextSize; i++)
	{
		// opcode: lea
		if ((TextSection[i] == uint8_t(0x4C) || TextSection[i] == uint8_t(0x48)) && TextSection[i + 1] == uint8_t(0x8D))
		{
			const uint8_t* StrPtr = *(uint32_t*)(TextSection + i + 3) + 7 + TextSection + i;

			if (StrPtr == StringAddress)
			{
				//std::cout << "Found Address: 0x" << (void*)(TextSection + i) << "\n";

				return { TextSection + i };
			}
		}
	}

	return nullptr;
}

inline MemAddress FindByWString(const wchar_t* RefStr)
{
	return FindByString<const wchar_t*>(RefStr);
}


