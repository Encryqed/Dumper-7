
#include "TmpUtils.h"
#include "PlatformWindows.h"
#include "Arch_x86.h"

// Private implementation to ensure that there is no accidental usage of platform-specific functions
namespace
{
	// Spills over into the rest of the file (ouside of this anonymous namespace). This may be ignored, as all other functions in this file are part of namepsace PlatformWindows;
	using namespace PlatformWindows;

	struct WindowsSectionInfo
	{
		const uintptr_t Imagebase = NULL;
		const IMAGE_SECTION_HEADER* SectionHeader = nullptr;

	public:
		inline bool IsValid() const
		{
			return Imagebase != NULL && SectionHeader != nullptr;
		}
	};
	static_assert(sizeof(WindowsSectionInfo) == sizeof(SectionInfo), "To allow interchangable clasting between SectionInfo types you must ensure that the size matches.");

	inline WindowsSectionInfo SectionInfoToWinSectionInfo(const SectionInfo& Info)
	{
		return std::bit_cast<WindowsSectionInfo>(Info);
	}
	inline SectionInfo WinSectionInfoToSectionInfo(const WindowsSectionInfo& Info)
	{
		return std::bit_cast<SectionInfo>(Info);
	}


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
		//union
		//{
		//	LIST_ENTRY InInitializationOrderLinks;
		//	LIST_ENTRY InProgressLinks;
		//};
		PVOID DllBase;
		PVOID EntryPoint;
		ULONG SizeOfImage;
		UNICODE_STRING FullDllName;
		UNICODE_STRING BaseDllName;
	};

	inline _TEB* _NtCurrentTeb()
	{
#if defined(_WIN64)
		return reinterpret_cast<struct _TEB*>(__readgsqword(((LONG)__builtin_offsetof(NT_TIB, Self))));
#elif defined(_WIN32)
		return reinterpret_cast<struct _TEB*>(__readfsdword(((LONG)__builtin_offsetof(NT_TIB, Self))));
#endif // _WIN32
	}

	inline PEB* GetPEB()
	{
		return reinterpret_cast<TEB*>(_NtCurrentTeb())->ProcessEnvironmentBlock;
	}

	inline const LDR_DATA_TABLE_ENTRY* GetModuleLdrTableEntry(const char* SearchModuleName)
	{
		const PEB* Peb = GetPEB();
		const PEB_LDR_DATA* Ldr = Peb->Ldr;

		int NumEntriesLeft = Ldr->Length;

		const std::string LowercaseSearchModuleName = Utils::StrToLower(SearchModuleName);

		for (const LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
		{
			const LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<const LDR_DATA_TABLE_ENTRY*>(P);

			const std::wstring WideModuleName(Entry->BaseDllName.Buffer, Entry->BaseDllName.Length >> 1);
			const std::string ModuleName = std::string(WideModuleName.begin(), WideModuleName.end());

			if (Utils::StrToLower(ModuleName) == LowercaseSearchModuleName)
				return Entry;
		}

		return nullptr;
	}

	inline std::pair<uintptr_t, uintptr_t> GetImageBaseAndSize(const char* const ModuleName = nullptr)
	{
		const uintptr_t ImageBase = GetModuleBase(ModuleName);
		const PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew);

		return { ImageBase, NtHeader->OptionalHeader.SizeOfImage };
	}

	inline const IMAGE_SECTION_HEADER* IterateAllSectionObjects(const uintptr_t ImageBase, const std::function<bool(const IMAGE_SECTION_HEADER*)>& Callback)
	{
		if (ImageBase == 0)
			return nullptr;

		const PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase);
		const PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + DosHeader->e_lfanew);

		if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
			return nullptr;

		PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeaders);

		DWORD TextSize = 0;

		for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
		{
			const IMAGE_SECTION_HEADER* CurrentSection = &Sections[i];

			if (Callback(CurrentSection))
				return CurrentSection;
		}

		return nullptr;
	}

	/* Returns the base address of the section and it's size */
	inline std::pair<uintptr_t, DWORD> GetSectionByName(uintptr_t ImageBase, const std::string& ReqestedSectionName)
	{
		const IMAGE_SECTION_HEADER* Section = IterateAllSectionObjects(ImageBase, [ReqestedSectionName](const IMAGE_SECTION_HEADER* Section) -> bool
			{
				return reinterpret_cast<const char*>(Section->Name) == ReqestedSectionName;
			});

		if (Section)
			return { (ImageBase + Section->VirtualAddress), Section->Misc.VirtualSize };
		
		return { NULL, 0 };
	}

	inline uint32_t GetAlignedSizeWithOffsetFromEnd(const uint32_t SizeToAlign, const uint32_t Alignment, const uint32_t OffsetFromEnd)
	{
		return Align((SizeToAlign - (Alignment - 1) - OffsetFromEnd), Alignment);
	}

	inline const PIMAGE_THUNK_DATA GetImportAddress(const uintptr_t ModuleBase, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		/* Get the module importing the function */
		const PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

		if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return nullptr;

		PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<const PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

		if (!NtHeader)
			return nullptr;

		const PIMAGE_IMPORT_DESCRIPTOR ImportTable = reinterpret_cast<const PIMAGE_IMPORT_DESCRIPTOR>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		//std::cerr << "ModuleName: " << (SearchModuleName ? SearchModuleName : "Default") << std::endl;

		const std::string LowercaseSearchModuleName = Utils::StrToLower(ModuleToImportFrom);

		/* Loop all modules and if we found the right one, loop all imports to get the one we need */
		for (const IMAGE_IMPORT_DESCRIPTOR* Import = ImportTable; Import && Import->Characteristics != 0x0; Import++)
		{
			if (Import->Name == 0xFFFF)
				continue;

			const char* Name = reinterpret_cast<const char*>(ModuleBase + Import->Name);

			//std::cerr << "Name: " << str_tolower(Name) << std::endl;

			if (Utils::StrToLower(Name) != LowercaseSearchModuleName)
				continue;

			PIMAGE_THUNK_DATA NameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(ModuleBase + Import->OriginalFirstThunk);
			PIMAGE_THUNK_DATA FuncThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(ModuleBase + Import->FirstThunk);

			while (!IsBadReadPtr(NameThunk)
				&& !IsBadReadPtr(FuncThunk)
				&& !IsBadReadPtr(ModuleBase + NameThunk->u1.AddressOfData)
				&& !IsBadReadPtr(FuncThunk->u1.AddressOfData))
			{
				/*
				* A functin might be imported using the Ordinal (Index) of this function in the modules export-table
				*
				* The name could probably be retrieved by looking up this Ordinal in the Modules export-name-table
				*/
				if ((NameThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) != 0) // No ordinal
				{
					NameThunk++;
					FuncThunk++;
					continue; // Maybe Handle this in the future
				}

				/* Get Import data for this function */
				PIMAGE_IMPORT_BY_NAME NameData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(ModuleBase + NameThunk->u1.ForwarderString);
				PIMAGE_IMPORT_BY_NAME FunctionData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(FuncThunk->u1.AddressOfData);

				//std::cerr << "IMPORT: " << std::string(NameData->Name) << std::endl;

				if (std::string(NameData->Name) == SearchFunctionName)
					return FuncThunk;

				NameThunk++;
				FuncThunk++;
			}
		}

		return nullptr;
	}

	std::pair<uintptr_t, uint32_t> GetSearchStartAndRangeBasedOnOverrides(const uintptr_t ModuleBase, const IMAGE_SECTION_HEADER* SectionHeader, const uintptr_t StartAddress, int32_t Range)
	{
		if (SectionHeader->VirtualAddress == NULL || SectionHeader->Misc.VirtualSize == 0x0)
			return { NULL, 0x0 };

		const uintptr_t SectionStartAddress = ModuleBase + SectionHeader->VirtualAddress;
		const uintptr_t SectionEndAddress = SectionStartAddress + SectionHeader->Misc.VirtualSize;

		uint32_t SearchRange = (Range > 0x0 && Range < SectionHeader->Misc.VirtualSize) ? Range : SectionHeader->Misc.VirtualSize;
		uintptr_t SearchStartAddress = SectionStartAddress;

		// Check if this section contains the StartAddress
		if (StartAddress != NULL && StartAddress > SectionStartAddress)
		{
			// This section does not contain any address greater than StartAddress
			if (StartAddress >= SectionEndAddress)
				return { NULL, 0x0 };

			SearchStartAddress = StartAddress;
			SearchRange = SectionEndAddress - StartAddress;
		}

		return { SearchStartAddress, SearchRange };
	}

	bool IsInAnySection(const uintptr_t Address, const DWORD OptionalRequiredCharacteristics = 0)
	{
		const auto ModuleBase = PlatformWindows::GetModuleBase();

		bool bFound = false;
		IterateAllSectionObjects(ModuleBase, [&bFound, Address, ModuleBase, OptionalRequiredCharacteristics](const IMAGE_SECTION_HEADER* SectionHeader) -> bool
			{
				const uintptr_t SectionStart = ModuleBase + SectionHeader->VirtualAddress;
				const uintptr_t SectionEnd = SectionStart + SectionHeader->Misc.VirtualSize;

				if (Address >= SectionStart && Address < SectionEnd)
				{
					bFound = (SectionHeader->Characteristics & OptionalRequiredCharacteristics) == OptionalRequiredCharacteristics;
					return true;
				}

				return false;
			});

		return bFound;
	}

	bool IsInAnySection(const void* Address, const DWORD OptionalRequiredCharacteristics = 0)
	{
		return IsInAnySection(reinterpret_cast<uintptr_t>(Address), OptionalRequiredCharacteristics);
	}
}


void* WindowsPrivateImplHelper::FinAlignedValueInRangeImpl(const void* ValuePtr, ValueCompareFuncType ComparisonFunction, const int32_t ValueTypeSize, const int32_t Alignment, uintptr_t StartAddress, uint32_t Range)
{
	for (uint32_t i = 0x0; i <= GetAlignedSizeWithOffsetFromEnd(Range, Alignment, ValueTypeSize); i += Alignment)
	{
		void* TypedPtr = reinterpret_cast<void*>(StartAddress + i);


		if (ComparisonFunction(ValuePtr, TypedPtr))
			return TypedPtr;
	}

	return nullptr;
}

void* WindowsPrivateImplHelper::FindAlignedValueInSectionImpl(const SectionInfo& Info, const void* ValuePtr, ValueCompareFuncType ComparisonFunction, const int32_t ValueTypeSize, const int32_t Alignment)
{
	const WindowsSectionInfo WinSectionInfo = SectionInfoToWinSectionInfo(Info);

	const uint32_t Range = WinSectionInfo.SectionHeader->Misc.VirtualSize;
	const uintptr_t SectionBaseAddrss = WinSectionInfo.Imagebase + WinSectionInfo.SectionHeader->VirtualAddress;

	return FinAlignedValueInRangeImpl(ValuePtr, ComparisonFunction, ValueTypeSize, Alignment, SectionBaseAddrss, Range);
}

void* WindowsPrivateImplHelper::FindAlignedValueInAllSectionsImpl(const void* ValuePtr, ValueCompareFuncType ComparisonFunction, const int32_t ValueTypeSize, const int32_t Alignment, const uintptr_t StartAddress, int32_t Range, const char* const ModuleName)
{
	const auto ModuleBase = GetModuleBase(ModuleName);

	void* Result = nullptr;
	auto FindStringInSection = [&Result, &Range, StartAddress, ModuleBase, ValuePtr, ComparisonFunction, ValueTypeSize, Alignment](const IMAGE_SECTION_HEADER* SectionHeader) -> bool
	{
		const auto [SearchStartAddress, SearchRange] = GetSearchStartAndRangeBasedOnOverrides(ModuleBase, SectionHeader, StartAddress, Range);
	
		if (SearchStartAddress == NULL || SearchRange == 0x0)
			return false;

		if (Range > 0x0)
			Range -= SearchRange;

		Result = FinAlignedValueInRangeImpl(ValuePtr, ComparisonFunction, ValueTypeSize, Alignment, SearchStartAddress, SearchRange);

		return Result != nullptr;
	};

	IterateAllSectionObjects(ModuleBase, FindStringInSection);

	return Result;
}



uintptr_t PlatformWindows::GetModuleBase(const char* const ModuleName)
{
	if (ModuleName == nullptr)
		return reinterpret_cast<uintptr_t>(GetPEB()->ImageBaseAddress);

	return reinterpret_cast<uintptr_t>(GetModuleLdrTableEntry(ModuleName)->DllBase);
}

uintptr_t PlatformWindows::GetOffset(const uintptr_t Address, const char* const ModuleName)
{
	return (Address - GetModuleBase(ModuleName));
}

uintptr_t PlatformWindows::GetOffset(const void* Address, const char* const ModuleName)
{
	return GetOffset(reinterpret_cast<const uintptr_t>(Address), ModuleName);
}

SectionInfo PlatformWindows::GetSectionInfo(const std::string& SectionName, const char* const ModuleName)
{
	const uintptr_t ModuleBase = GetModuleBase(ModuleName);

	WindowsSectionInfo WinSectionInfo = { .Imagebase = ModuleBase };

	WinSectionInfo.SectionHeader = IterateAllSectionObjects(ModuleBase, [SectionName](const IMAGE_SECTION_HEADER* Section) -> bool
		{
			return reinterpret_cast<const char*>(Section->Name) == SectionName;
		});
	
	return WinSectionInfoToSectionInfo(WinSectionInfo);
}

void* PlatformWindows::IterateSectionWithCallback(const SectionInfo& Info, const std::function<bool(void* Address)>& Callback, uint32_t Granularity, uint32_t OffsetFromEnd)
{
	const WindowsSectionInfo WinSectionInfo = SectionInfoToWinSectionInfo(Info);

	if (!WinSectionInfo.IsValid())
		return nullptr;

	const uintptr_t SectionBaseAddrss = WinSectionInfo.Imagebase + WinSectionInfo.SectionHeader->VirtualAddress;
	const uint32_t SectionIterationSize = GetAlignedSizeWithOffsetFromEnd(WinSectionInfo.SectionHeader->Misc.VirtualSize, Granularity, OffsetFromEnd);

	for (uintptr_t CurrentAddress = SectionBaseAddrss; CurrentAddress < (SectionBaseAddrss + SectionIterationSize); CurrentAddress += Granularity)
	{
		if (Callback(reinterpret_cast<void*>(CurrentAddress)))
			return reinterpret_cast<void*>(CurrentAddress);
	}

	return nullptr;
}

void* PlatformWindows::IterateAllSectionsWithCallback(const std::function<bool(void* Address)>& Callback, uint32_t Granularity, uint32_t OffsetFromEnd, const char* const ModuleName)
{
	void* Result = nullptr;

	const uintptr_t ModuleBase = GetModuleBase(ModuleName);

	IterateAllSectionObjects(ModuleBase, [ModuleBase, &Result, &Callback, Granularity, OffsetFromEnd](const IMAGE_SECTION_HEADER* Section) -> bool
		{
			const WindowsSectionInfo WinSectionInfo = { ModuleBase, Section };

			if (void* Address = IterateSectionWithCallback(WinSectionInfoToSectionInfo(WinSectionInfo), Callback, Granularity, OffsetFromEnd))
			{
				Result = Address;
				return true;
			}

			return false;
		});

	return Result;
}


bool PlatformWindows::IsAddressInAnyModule(const uintptr_t Address)
{
	return IsAddressInAnyModule(reinterpret_cast<const void*>(Address));
}

bool PlatformWindows::IsAddressInAnyModule(const void* Address)
{
	const PEB* Peb = GetPEB();
	const PEB_LDR_DATA* Ldr = Peb->Ldr;

	int NumEntriesLeft = Ldr->Length;

	for (const LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
	{
		const LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<const LDR_DATA_TABLE_ENTRY*>(P);

		if (Address > Entry->DllBase && Address < (static_cast<uint8_t*>(Entry->DllBase) + Entry->SizeOfImage))
			return true;
	}

	return false;
}

bool PlatformWindows::IsAddressInProcessRange(const uintptr_t Address)
{
	const auto [ImageBase, ImageSize] = GetImageBaseAndSize();

	if (Address >= ImageBase && Address < (ImageBase + ImageSize))
		return true;

	return IsAddressInAnyModule(Address);
}
bool PlatformWindows::IsAddressInProcessRange(const void* Address)
{
	return IsAddressInProcessRange(reinterpret_cast<uintptr_t>(Address));
}
bool PlatformWindows::IsBadReadPtr(const uintptr_t Address)
{
	return IsBadReadPtr(reinterpret_cast<const void*>(Address));
}
bool PlatformWindows::IsBadReadPtr(const void* Address)
{
	if constexpr (!Is32Bit())
	{
		if (!Architecture_x86_64::IsValid64BitVirtualAddress(Address))
			return true;
	}

	MEMORY_BASIC_INFORMATION Mbi;

	if (VirtualQuery(Address, &Mbi, sizeof(Mbi)))
	{
		constexpr DWORD AccessibleMask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		constexpr DWORD InaccessibleMask = (PAGE_GUARD | PAGE_NOACCESS);

		return !(Mbi.Protect & AccessibleMask) || (Mbi.Protect & InaccessibleMask);
	}

	return true;
}

const void* PlatformWindows::GetAddressOfImportedFunction(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	const uintptr_t SearchModule = GetModuleBase(SearchModuleName);

	return GetImportAddress(SearchModule, ModuleToImportFrom, SearchFunctionName);
}
const void* PlatformWindows::GetAddressOfImportedFunctionFromAnyModule(const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	const PEB* Peb = GetPEB();
	const PEB_LDR_DATA* Ldr = Peb->Ldr;

	int NumEntriesLeft = Ldr->Length;

	for (const LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
	{
		const LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<const LDR_DATA_TABLE_ENTRY*>(P);

		const PIMAGE_THUNK_DATA Import = GetImportAddress(reinterpret_cast<uintptr_t>(Entry->DllBase), ModuleToImportFrom, SearchFunctionName);

		if (Import)
			return reinterpret_cast<const void*>(Import->u1.AddressOfData);
	}

	return nullptr;
}

const void* PlatformWindows::GetAddressOfExportedFunction(const char* SearchModuleName, const char* SearchFunctionName)
{
	/* Get the module the function was exported from */
	const uintptr_t ModuleBase = GetModuleBase(SearchModuleName);
	const IMAGE_DOS_HEADER* DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

	if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	const IMAGE_NT_HEADERS* NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

	if (!NtHeader)
		return nullptr;

	/* Get the table of functions exported by the module */
	const IMAGE_EXPORT_DIRECTORY* ExportTable = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	const DWORD* NameOffsets = reinterpret_cast<const DWORD*>(ModuleBase + ExportTable->AddressOfNames);
	const DWORD* FunctionOffsets = reinterpret_cast<const DWORD*>(ModuleBase + ExportTable->AddressOfFunctions);

	const WORD* Ordinals = reinterpret_cast<const WORD*>(ModuleBase + ExportTable->AddressOfNameOrdinals);

	/* Iterate all names and return the function if the name matches what we're looking for */
	for (int i = 0; i < ExportTable->NumberOfFunctions; i++)
	{
		const WORD NameIndex = Ordinals[i];
		const char* Name = reinterpret_cast<const char*>(ModuleBase + NameOffsets[NameIndex]);

		if (strcmp(SearchFunctionName, Name) == 0)
			return reinterpret_cast<void*>(ModuleBase + FunctionOffsets[i]);
	}

	return nullptr;
}

template<bool bShouldResolve32BitJumps>
std::pair<const void*, int32_t> PlatformWindows::IterateVTableFunctions(void** VTable, const std::function<bool(const uint8_t* Address, int32_t Index)>& CallBackForEachFunc, int32_t NumFunctions, int32_t OffsetFromStart)
{
	[[maybe_unused]] auto Resolve32BitRelativeJump = [](const void* FunctionPtr) -> const uint8_t*
	{
		if constexpr (bShouldResolve32BitJumps)
		{
			const uint8_t* Address = reinterpret_cast<const uint8_t*>(FunctionPtr);
			if (*Address == 0xE9)
			{
				const uint8_t* Ret = ((Address + 5) + *reinterpret_cast<const int32_t*>(Address + 1));

				if (IsAddressInProcessRange(Ret))
					return Ret;
			}
		}

		return reinterpret_cast<const uint8_t*>(FunctionPtr);
	};


	if (!CallBackForEachFunc)
		return { nullptr, -1 };

	for (int i = 0; i < 0x150; i++)
	{
		const uintptr_t CurrentFuncAddress = reinterpret_cast<uintptr_t>(VTable[i]);

		if (CurrentFuncAddress == NULL || !IsAddressInProcessRange(CurrentFuncAddress))
			break;

		const uint8_t* ResolvedAddress = Resolve32BitRelativeJump(reinterpret_cast<const uint8_t*>(CurrentFuncAddress));

		if (CallBackForEachFunc(ResolvedAddress, i))
			return { ResolvedAddress, i };
	}

	return { nullptr, -1 };
}


void* PlatformWindows::FindPattern(const char* Signature, const uint32_t Offset, const bool bSearchAllSections, const uintptr_t StartAddress, const char* const ModuleName)
{
	const auto ModuleBase = GetModuleBase(ModuleName);

	void* Result = nullptr;
	auto FindPatternInRangeLambda = [&Result, ModuleBase, Signature, Offset, StartAddress](const IMAGE_SECTION_HEADER* SectionHeader) -> bool
	{
		const auto [SearchStartAddress, SearchRange] = GetSearchStartAndRangeBasedOnOverrides(ModuleBase, SectionHeader, StartAddress, 0x0);

		if (SearchStartAddress == NULL || SearchRange == 0x0)
			return false;

		Result = FindPatternInRange(Signature, reinterpret_cast<const uint8_t*>(SearchStartAddress), SearchRange, Offset != 0x0, Offset);

		return Result != nullptr;
	};

	if (bSearchAllSections)
	{
		IterateAllSectionObjects(ModuleBase, FindPatternInRangeLambda);
	}
	else
	{
		const WindowsSectionInfo WinSectionInfo = SectionInfoToWinSectionInfo(GetSectionInfo(".text", ModuleName));

		const uint32_t Range = WinSectionInfo.SectionHeader->Misc.VirtualSize;
		const uintptr_t SectionBaseAddrss = WinSectionInfo.Imagebase + WinSectionInfo.SectionHeader->VirtualAddress;

		return FindPatternInRange(Signature, reinterpret_cast<const uint8_t*>(SectionBaseAddrss), Range, Offset != 0x0, Offset);
	}

	return Result;
}

void* PlatformWindows::FindPatternInRange(const char* Signature, const void* Start, const uintptr_t Range, const bool bRelative, const uint32_t Offset)
{
	static auto PatternToByte = [](const char* pattern) -> std::vector<int>
	{
		std::vector<int> Bytes;

		const auto Start = const_cast<char*>(pattern);
		const auto End = const_cast<char*>(pattern) + strlen(pattern);

		for (auto Current = Start; Current < End; ++Current)
		{
			if (*Current == '?')
			{
				++Current;

				if (*Current == '?')
					++Current;

				Bytes.push_back(-1);
			}
			else
			{
				Bytes.push_back(strtoul(Current, &Current, 16));
			}
		}

		return Bytes;
	};

	return FindPatternInRange(PatternToByte(Signature), Start, Range, bRelative, Offset);
}

void* PlatformWindows::FindPatternInRange(const char* Signature, const uintptr_t Start, const uintptr_t Range, const bool bRelative, const uint32_t Offset)
{
	return FindPatternInRange(Signature, reinterpret_cast<void*>(Start), Range, bRelative, Offset);
}

void* PlatformWindows::FindPatternInRange(std::vector<int>&& Signature, const void* Start, const uintptr_t Range, const bool bRelative, uint32_t Offset, const uint32_t SkipCount)
{
	const auto PatternLength = Signature.size();
	const auto PatternBytes = Signature.data();

	for (int i = 0; i < (Range - PatternLength); i++)
	{
		bool bFound = true;
		int CurrentSkips = 0;

		for (auto j = 0ul; j < PatternLength; ++j)
		{
			if (static_cast<const uint8_t*>(Start)[i + j] != PatternBytes[j] && PatternBytes[j] != -1)
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

			uintptr_t Address = reinterpret_cast<uintptr_t>(Start) + i;
			if (bRelative)
			{
				if (Offset == -1)
					Offset = PatternLength;

				Address = ((Address + Offset + 4) + *reinterpret_cast<int32_t*>(Address + Offset));
			}
			return reinterpret_cast<void*>(Address);
		}
	}

	return nullptr;
}

/* Slower than FindByString */
template<bool bCheckIfLeaIsStrPtr, typename CharType>
void* PlatformWindows::FindByStringInAllSections(const CharType* RefStr,const uintptr_t StartAddress, int32_t Range, const bool bSearchOnlyExecutableSections, const char* const ModuleName)
{
	static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "FindByStringInAllSections only supports 'char' and 'wchar_t', but was called with other type.");

	const auto ModuleBase = GetModuleBase(ModuleName);

	void* Result = nullptr;
	auto FindStringInSection = [&Result, &Range, StartAddress, ModuleBase, bSearchOnlyExecutableSections, RefStr](const IMAGE_SECTION_HEADER* SectionHeader) -> bool
	{
		if (bSearchOnlyExecutableSections && !(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE))
			return false;

		const auto [SearchStartAddress, SearchRange] = GetSearchStartAndRangeBasedOnOverrides(ModuleBase, SectionHeader, StartAddress, Range);

		if (SearchStartAddress == NULL || SearchRange == 0x0)
			return false;

		if (Range > 0x0)
			Range -= SearchRange;

		// Make sure we don't try to read beyond the limit. This might cause string refs at the very end of a search Range not to be found.
		constexpr auto InstructionBytesLength = 0x7;

		Result = FindStringInRange<bCheckIfLeaIsStrPtr, CharType>(RefStr, SearchStartAddress, (SearchRange - InstructionBytesLength));

		return Result != nullptr;
	};

	IterateAllSectionObjects(ModuleBase, FindStringInSection);

	return Result;
}

template<bool bCheckIfLeaIsStrPtr, typename CharType>
inline void* PlatformWindows::FindStringInRange(const CharType* RefStr, const uintptr_t StartAddress, const int32_t Range)
{
	uint8_t* const SearchStart = reinterpret_cast<uint8_t*>(StartAddress);

	const int32_t RefStrLen = StrlenHelper(RefStr);

	for (uintptr_t i = 0; i < Range; i++)
	{
#if defined(_WIN64)
		// opcode: lea
		if ((SearchStart[i] == uint8_t(0x4C) || SearchStart[i] == uint8_t(0x48)) && SearchStart[i + 1] == uint8_t(0x8D))
		{
			const uintptr_t StrPtr = Architecture_x86_64::Resolve32BitRelativeLea(reinterpret_cast<uintptr_t>(SearchStart + i));

			if (!IsAddressInProcessRange(StrPtr))
				continue;

			if (!IsInAnySection(StrPtr, IMAGE_SCN_MEM_READ) && IsBadReadPtr(StrPtr))
				continue;


			if (StrnCmpHelper(RefStr, reinterpret_cast<const CharType*>(StrPtr), RefStrLen))
				return { SearchStart + i };

			if constexpr (bCheckIfLeaIsStrPtr)
			{
				const CharType* StrPtrContentFirst8Bytes = *reinterpret_cast<const CharType* const*>(StrPtr);

				if (!IsAddressInProcessRange(StrPtrContentFirst8Bytes))
					continue;

				if (StrnCmpHelper(RefStr, StrPtrContentFirst8Bytes, RefStrLen))
					return { SearchStart + i };
			}
		}
#elif defined(_WIN32)
		// opcode: push
		if ((SearchStart[i] == uint8_t(0x68)))
		{
			const uintptr_t StrPtr = Architecture_x86_64::Resolve32BitRelativePush(reinterpret_cast<uintptr_t>(SearchStart + i));

			if (!IsAddressInProcessRange(StrPtr))
				continue;

			if (!IsBadReadPtr(StrPtr))
				continue;

			if (StrnCmpHelper(RefStr, reinterpret_cast<const CharType*>(StrPtr), RefStrLen))
			{
				return { SearchStart + i };
			}
		}
#endif
	}

	return nullptr;
}



/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
*
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
template void* PlatformWindows::FindByStringInAllSections<false, char>(const char*, const uintptr_t, int32_t, const bool, const char* const);
template void* PlatformWindows::FindByStringInAllSections<false, wchar_t>(const wchar_t*, const uintptr_t, int32_t, const bool, const char* const);
template void* PlatformWindows::FindByStringInAllSections<true, char>(const char*, const uintptr_t, int32_t, const bool, const char* const);
template void* PlatformWindows::FindByStringInAllSections<true, wchar_t>(const wchar_t*, const uintptr_t, int32_t, const bool, const char* const);

template std::pair<const void*, int32_t> PlatformWindows::IterateVTableFunctions<true>(void**, const std::function<bool(const uint8_t*, int32_t)>&, int32_t, int32_t);
template std::pair<const void*, int32_t> PlatformWindows::IterateVTableFunctions<false>(void**, const std::function<bool(const uint8_t*, int32_t)>&, int32_t, int32_t);

