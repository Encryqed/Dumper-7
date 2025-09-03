
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
				reinterpret_cast<const char*>(Section->Name) == ReqestedSectionName;
			});

		if (Section)
			return { (ImageBase + Section->VirtualAddress), Section->Misc.VirtualSize };
		
		return { NULL, 0 };
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

	return std::bit_cast<SectionInfo>(WinSectionInfo);
}

void* PlatformWindows::IterateSectionWithCallback(const SectionInfo& Info, const std::function<bool(void* Address)>& Callback, uint32_t Granularity, uint32_t OffsetFromEnd)
{
	const WindowsSectionInfo WinSectionInfo = std::bit_cast<WindowsSectionInfo>(Info);

	if (!WinSectionInfo.IsValid())
		return nullptr;

	const uintptr_t SectionBaseAddrss = WinSectionInfo.Imagebase + WinSectionInfo.SectionHeader->VirtualAddress;
	const uint32_t SizeToAlign = WinSectionInfo.SectionHeader->SizeOfRawData - (Granularity - 1) - OffsetFromEnd;
	const uint32_t SectionIterationSize = Align(SizeToAlign, Granularity);

	for (uintptr_t CurrentAddress = SectionBaseAddrss; CurrentAddress < (CurrentAddress + SectionIterationSize); CurrentAddress += Granularity)
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

			if (void* Address = IterateSectionWithCallback(std::bit_cast<SectionInfo>(WinSectionInfo), Callback, Granularity, OffsetFromEnd))
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
	/*
	* 
	* TODO: MAKE THIS if constexpr (Is32Bit())
	* 
	* 
	*/
#if defined(_WIN64)
		// we only really do this on x86_64 ^^
		if (!Architecture_x86_64::IsValidVirtualAddress(Address))
			return true;
#endif // _WIN64

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


/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
*
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
namespace
{
	inline void Unused()
	{
		IterateVTableFunctions<true>(nullptr, {});
		IterateVTableFunctions<false>(nullptr, {});
	}
}