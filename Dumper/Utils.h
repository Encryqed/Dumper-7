#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

struct CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
};

struct TEB
{
	NT_TIB NtTib;
	PVOID EnvironmentPointer;
	CLIENT_ID ClientId;
	PVOID ActiveRpcHandle;
	PVOID ThreadLocalStoragePointer;
	struct PEB* ProcessEnvironmentBlock;
};

struct PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	HANDLE ShutdownThreadId;
};

struct PEB
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN SpareBits : 1;
		};
	};
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PEB_LDR_DATA* Ldr;
};

struct UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWCH Buffer;
};

struct LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	union
	{
		LIST_ENTRY InInitializationOrderLinks;
		LIST_ENTRY InProgressLinks;
	};
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
}; 

inline _TEB* _NtCurrentTeb()
{
	return (struct _TEB*)__readgsqword(((LONG)__builtin_offsetof(NT_TIB, Self)));
}

inline uintptr_t GetModuleSize()
{
	PEB* ProcessEnvironmentBlock = ((PEB*)((TEB*)((TEB*)_NtCurrentTeb())->ProcessEnvironmentBlock));
	PEB_LDR_DATA* Ldr = ProcessEnvironmentBlock->Ldr;

	int i = 0;

	LIST_ENTRY** Start = (LIST_ENTRY**)&Ldr->InMemoryOrderModuleList;
	for (LIST_ENTRY* P = *Start; P; P = P->Flink)
	{
		LDR_DATA_TABLE_ENTRY* A = (LDR_DATA_TABLE_ENTRY*)P;

		if (!A || i++ == 0x20)
			break;

		std::cout << "DllBase: " << A->DllBase << std::endl;
		std::cout << "EntryPoint: " << A->EntryPoint << std::endl;
		std::cout << "Size: " << std::hex << A->SizeOfImage << std::endl;

		std::wstring DllName(A->FullDllName.Buffer, A->FullDllName.Length >> 1);
		std::string DDLName = std::string(DllName.begin(), DllName.end());

		std::cout << DDLName << "\n" << std::endl;
	}

	return 0;
}

inline uintptr_t GetImageBase()
{
	PEB* ProcessEnvironmentBlock = ((PEB*)((TEB*)((TEB*)_NtCurrentTeb())->ProcessEnvironmentBlock));
	return (uintptr_t)ProcessEnvironmentBlock->ImageBaseAddress;
}

inline uintptr_t GetOffset(void* Addr)
{
	static uintptr_t ImageBase = 0x0;

	if (ImageBase == 0x0)
		ImageBase = GetImageBase();

	uintptr_t AddrAsInt = reinterpret_cast<uintptr_t>(Addr);

	return AddrAsInt > ImageBase ? (AddrAsInt - ImageBase) : 0x0;
}

inline bool IsInProcessRange(uintptr_t Address)
{
	uintptr_t ImageBase = GetImageBase();
	PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew);

	return Address > ImageBase && Address < (NtHeader->OptionalHeader.SizeOfImage + ImageBase);
}

static bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi;

	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		constexpr DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			b = true;

		return b;
	}

	return true;
};

static inline void* FindPatternInRange(std::vector<int>&& Signature, uint8_t* Start, uintptr_t Range, bool bRelative = false, uint32_t Offset = 0, int SkipCount = 0)
{
	const auto PatternLength = Signature.size();
	const auto PatternBytes = Signature.data();

	for (int i = 0; i < (Range - PatternLength); i++)
	{
		bool bFound = true;
		int CurrentSkips = 0;

		for (auto j = 0ul; j < PatternLength; ++j)
		{
			if (Start[i + j] != PatternBytes[j] && PatternBytes[j] != -1)
			{
				bFound = false;
				break;
			}
		}
		if (bFound)
		{
			if (CurrentSkips != SkipCount)
			{
				CurrentSkips++;
				continue;
			}

			uintptr_t Address = uintptr_t(Start + i);
			if (bRelative)
			{
				if (Offset == -1)
					Offset = PatternLength;

				Address = ((Address + Offset + 4) + *(int32_t*)(Address + Offset));
			}
			return (void*)Address;
		}
	}

	return nullptr;
}

static inline void* FindPatternInRange(const char* Signature, uint8_t* Start, uintptr_t Range, bool bRelative = false, uint32_t Offset = 0)
{
	static auto patternToByte = [](const char* pattern) -> std::vector<int>
	{
		auto Bytes = std::vector<int>{};
		const auto Start = const_cast<char*>(pattern);
		const auto End = const_cast<char*>(pattern) + strlen(pattern);

		for (auto Current = Start; Current < End; ++Current)
		{
			if (*Current == '?')
			{
				++Current;
				if (*Current == '?') ++Current;
				Bytes.push_back(-1);
			}
			else { Bytes.push_back(strtoul(Current, &Current, 16)); }
		}
		return Bytes;
	};

	return FindPatternInRange(patternToByte(Signature), Start, Range, bRelative, Offset);
}

static inline void* FindPattern(const char* Signature, uint32_t Offset = 0, bool bSearchAllSegments = false)
{
	uint8_t* ImageBase = (uint8_t*)GetImageBase();

	const auto DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	const auto NtHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);

	const auto SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;

	if (!bSearchAllSegments)
	{
		PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeaders);

		uint8_t* TextSection = nullptr;
		DWORD TextSize = 0;

		for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
		{
			IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

			std::string SectionName = (const char*)CurrentSection.Name;

			if (SectionName == ".text" && !TextSection)
			{
				TextSection = (ImageBase + CurrentSection.VirtualAddress);
				TextSize = CurrentSection.Misc.VirtualSize;
			}
		}

		return FindPatternInRange(Signature, TextSection, TextSize, Offset != 0x0, Offset);
	}

	return FindPatternInRange(Signature, ImageBase, SizeOfImage, Offset != 0x0, Offset);
}

struct MemAddress
{
public:
	uint8_t* Address;

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

	static bool IsFunctionRet(uint8_t* Address)
	{
		int Align = 0x10 - (uintptr_t(Address) % 0x10);
		//if (Opcode == RET && (OpcodeBefore is a POP opcode || OpcodeTwoBefore is a different POP Opcode)
		return Address[0] == 0xC3 && Address[Align] == 0x40 && ((Address[-1] >= 0x58 && Address[-1] <= 0x5F) || (Address[-2] == 0x41 && (Address[-1] >= 0x58 && Address[-1] <= 0x5F)));
	}

public:
	inline MemAddress(void* Addr)
		: Address((uint8_t*)Addr)
	{
	}
	inline MemAddress(uintptr_t Addr)
		: Address((uint8_t*)Addr)
	{
	}
	operator bool()
	{
		return Address != nullptr;
	}
	explicit operator void*()
	{
		return Address;
	}
	explicit operator uintptr_t()
	{
		return uintptr_t(Address);
	}

	inline MemAddress operator+(int Value) const
	{
		return Address + Value;
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

	inline void* RelativePattern(const char* Pattern, int32_t Range, int32_t Relative = 0)
	{
		if (!Address)
			return nullptr;

		return FindPatternInRange(Pattern, Address, Range, Relative != 0, Relative);
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
					uint8_t* CallAddr = Address + i + j;

					void* Func = CallAddr + *reinterpret_cast<int32*>(CallAddr + 1) + 5; /*Addr + Offset + 5*/

					if ((uint64_t(Func) % 0x4) != 0x0)
						continue;
						
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
	uintptr_t ImageBase = GetImageBase();
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
		if constexpr (std::is_same<Type, const char*>())
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
			const uint8_t* StrPtr = *(int32_t*)(TextSection + i + 3) + 7 + TextSection + i;

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

/* Slower than FindByString */
template<typename Type = const char*>
inline MemAddress FindByStringInAllSections(Type RefStr, void* StartAddress = nullptr)
{
	uintptr_t ImageBase = GetImageBase();
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);

	const DWORD SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

	uint8_t* SearchStart = StartAddress ? reinterpret_cast<uint8_t*>(StartAddress) : reinterpret_cast<uint8_t*>(ImageBase);
	DWORD SearchRange = SizeOfImage;

	for (int i = 0; i < SearchRange; i++)
	{
		// opcode: lea
		if ((SearchStart[i] == uint8_t(0x4C) || SearchStart[i] == uint8_t(0x48)) && SearchStart[i + 1] == uint8_t(0x8D))
		{
			const uint8_t* StrPtr = *(int32_t*)(SearchStart + i + 3) + 7 + SearchStart + i;

			if (!IsInProcessRange((uintptr_t)StrPtr))
				continue;

			if constexpr (std::is_same<Type, const char*>())
			{
				if (strcmp((const char*)RefStr, (const char*)StrPtr) == 0)
				{
					//std::cout << "FoundStr ref: " << (const char*)(SearchStart + i) << "\n";

					return { SearchStart + i };
				}
			}
			else
			{
				auto a = std::wstring((const wchar_t*)StrPtr);

				if (wcscmp((const wchar_t*)RefStr, (const wchar_t*)StrPtr) == 0) 
				{
					//std::wcout << L"FoundStr wref: " << (const wchar_t*)(SearchStart + i) << L"\n";

					return { SearchStart + i };
				}
			}
		}
	}

	return nullptr;
}

/* Slower than FindByWString */
inline MemAddress FindByWStringInAllSections(const wchar_t* RefStr)
{
	return FindByStringInAllSections<const wchar_t*>(RefStr);
}



