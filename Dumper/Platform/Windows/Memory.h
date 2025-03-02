#pragma once

#include <Windows.h>
#include <utility>
#include <string>
#include <vector>

#include "Utils.h"


namespace PlatformWindows
{
	constexpr const char* DefaultCodeSectionName = ".text";
	constexpr const char* DefaultDataSectionName = ".data";

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
		BYTE MoreFunnyPadding[0x3];
		HANDLE SsHandle;
		LIST_ENTRY InLoadOrderModuleList;
		LIST_ENTRY InMemoryOrderModuleList;
		LIST_ENTRY InInitializationOrderModuleList;
		PVOID EntryInProgress;
		BOOLEAN ShutdownInProgress;
		BYTE MoreFunnyPadding2[0x7];
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


	inline _TEB* GetCurrentTeb()
	{
		return reinterpret_cast<struct _TEB*>(__readgsqword(((LONG)__builtin_offsetof(NT_TIB, Self))));
	}

	inline PEB* GetPEB()
	{
		return reinterpret_cast<TEB*>(GetCurrentTeb())->ProcessEnvironmentBlock;
	}

	inline LDR_DATA_TABLE_ENTRY* GetModuleLdrTableEntry(const char* SearchModuleName)
	{
		PEB* Peb = GetPEB();
		PEB_LDR_DATA* Ldr = Peb->Ldr;

		int NumEntriesLeft = Ldr->Length;

		for (LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
		{
			LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(P);

			std::wstring WideModuleName(Entry->BaseDllName.Buffer, Entry->BaseDllName.Length >> 1);
			std::string ModuleName = std::string(WideModuleName.begin(), WideModuleName.end());

			if (str_tolower(ModuleName) == str_tolower(SearchModuleName))
				return Entry;
		}

		return nullptr;
	}

	inline void* GetModuleAddressByName(const char* SearchModuleName)
	{
		const LDR_DATA_TABLE_ENTRY* Entry = GetModuleLdrTableEntry(SearchModuleName);

		if (Entry)
			return Entry->DllBase;

		return nullptr;
	}

	/* Gets the address at which a pointer to an imported function is stored */
	inline PIMAGE_THUNK_DATA GetImportAddress(uintptr_t ModuleBase, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		/* Get the module importing the function */
		PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

		if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return nullptr;

		PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

		if (!NtHeader)
			return nullptr;

		PIMAGE_IMPORT_DESCRIPTOR ImportTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		//std::cout << "ModuleName: " << (SearchModuleName ? SearchModuleName : "Default") << std::endl;

		/* Loop all modules and if we found the right one, loop all imports to get the one we need */
		for (PIMAGE_IMPORT_DESCRIPTOR Import = ImportTable; Import && Import->Characteristics != 0x0; Import++)
		{
			if (Import->Name == 0xFFFF)
				continue;

			const char* Name = reinterpret_cast<const char*>(ModuleBase + Import->Name);

			//std::cout << "Name: " << str_tolower(Name) << std::endl;

			if (str_tolower(Name) != str_tolower(ModuleToImportFrom))
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
				const PIMAGE_IMPORT_BY_NAME NameData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(ModuleBase + NameThunk->u1.ForwarderString);
				const PIMAGE_IMPORT_BY_NAME FunctionData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(FuncThunk->u1.AddressOfData);

				//std::cout << "IMPORT: " << std::string(NameData->Name) << std::endl;

				if (std::string(NameData->Name) == SearchFunctionName)
					return FuncThunk;

				NameThunk++;
				FuncThunk++;
			}
		}

		return nullptr;
	}

	/* Gets the address at which a pointer to an imported function is stored */
	inline PIMAGE_THUNK_DATA GetImportAddress(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		const uintptr_t SearchModule = SearchModuleName ? reinterpret_cast<uintptr_t>(GetModuleAddressByName(SearchModuleName)) : GetModuleBase();

		return GetImportAddress(SearchModule, ModuleToImportFrom, SearchFunctionName);
	}

	/* Finds the import for a funciton and returns the address of the function from the imported module */
	inline void* GetAddressOfImportedFunction(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		PIMAGE_THUNK_DATA FuncThunk = GetImportAddress(SearchModuleName, ModuleToImportFrom, SearchFunctionName);

		if (!FuncThunk)
			return nullptr;

		return reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(FuncThunk->u1.AddressOfData);
	}

	inline void* GetAddressOfImportedFunctionFromAnyModule(const char* ModuleToImportFrom, const char* SearchFunctionName)
	{
		PEB* Peb = GetPEB();
		PEB_LDR_DATA* Ldr = Peb->Ldr;

		int NumEntriesLeft = Ldr->Length;

		for (LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
		{
			LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(P);

			PIMAGE_THUNK_DATA Import = GetImportAddress(reinterpret_cast<uintptr_t>(Entry->DllBase), ModuleToImportFrom, SearchFunctionName);

			if (Import)
				return reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(Import->u1.AddressOfData);
		}

		return nullptr;
	}

	/* Gets the address of an exported function */
	inline void* GetExportAddress(const char* SearchModuleName, const char* SearchFunctionName)
	{
		/* Get the module the function was exported from */
		const uintptr_t ModuleBase = reinterpret_cast<uintptr_t>(GetModuleAddressByName(SearchModuleName));
		const PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

		if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return nullptr;

		const PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

		if (!NtHeader)
			return nullptr;

		/* Get the table of functions exported by the module */
		const PIMAGE_EXPORT_DIRECTORY ExportTable = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

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

	inline uintptr_t GetModuleBase(const char* const ModuleName = nullptr)
	{
		if (ModuleName == nullptr)
			return reinterpret_cast<uintptr_t>(GetPEB()->ImageBaseAddress);

		return reinterpret_cast<uintptr_t>(GetModuleLdrTableEntry(ModuleName)->DllBase);
	}

	inline std::pair<uintptr_t, uintptr_t> GetModuleBaseAndSize(const char* const ModuleName = nullptr)
	{
		const uintptr_t ImageBase = GetModuleBase(ModuleName);
		const PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew);

		return { ImageBase, NtHeader->OptionalHeader.SizeOfImage };
	}

	inline uintptr_t GetOffset(const uintptr_t Address)
	{
		static uintptr_t ImageBase = 0x0;

		if (ImageBase == 0x0)
			ImageBase = GetModuleBase();

		return Address > ImageBase ? (Address - ImageBase) : 0x0;
	}

	inline uintptr_t GetOffset(const void* Address)
	{
		return GetOffset(reinterpret_cast<const uintptr_t>(Address));
	}

	inline bool IsAddrInAnyModule(const void* Address)
	{
		const auto* Peb = GetPEB();

		int NumEntriesLeft = Peb->Ldr->Length;

		for (const LIST_ENTRY* P = Peb->Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
		{
			const auto* Entry = reinterpret_cast<const LDR_DATA_TABLE_ENTRY*>(P);

			if (Address > Entry->DllBase && Address < (reinterpret_cast<PCHAR>(Entry->DllBase) + Entry->SizeOfImage))
				return true;
		}

		return false;
	}

	inline bool IsAddrInAnyModule(const uintptr_t Address)
	{
		return IsAddrInAnyModule(reinterpret_cast<void*>(Address));
	}

	inline bool IsInProcessRange(const uintptr_t Address)
	{
		const auto [ImageBase, ImageSize] = GetModuleBaseAndSize();

		if (Address >= ImageBase && Address < (ImageBase + ImageSize))
			return true;

		return IsAddrInAnyModule(Address);
	}

	inline bool IsInProcessRange(const void* Address)
	{
		return IsInProcessRange(reinterpret_cast<const uintptr_t>(Address));
	}

	inline bool IsBadReadPtr(const void* Ptr)
	{
		if (!IsValidVirtualAddress(reinterpret_cast<const uintptr_t>(Ptr)))
			return true;

		MEMORY_BASIC_INFORMATION Mbi;

		if (VirtualQuery(Ptr, &Mbi, sizeof(Mbi)))
		{
			constexpr DWORD AccessibleMask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
			constexpr DWORD InaccessibleMask = (PAGE_GUARD | PAGE_NOACCESS);

			return !(Mbi.Protect & AccessibleMask) || (Mbi.Protect & InaccessibleMask);
		}

		return true;
	};

	inline bool IsBadReadPtr(const uintptr_t Ptr)
	{
		return IsBadReadPtr(reinterpret_cast<const void*>(Ptr));
	}

	inline std::pair<const PIMAGE_SECTION_HEADER, DWORD> GetSectionHeader(uintptr_t ImageBase = 0)
	{
		if (ImageBase == NULL)
			ImageBase = GetModuleBase();

		const PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase);
		const PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + DosHeader->e_lfanew);

		const PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeaders);

		return { Sections, NtHeaders->FileHeader.NumberOfSections };
	}

	/* Returns the base address of th section and it's size */
	inline std::pair<uintptr_t, DWORD> GetSectionByName(const uintptr_t ImageBase, const std::string& ReqestedSectionName)
	{
		auto [Sections, NumSections] = GetSectionHeader(ImageBase);

		for (int i = 0; i < NumSections; i++)
		{
			const IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

			const std::string SectionName = reinterpret_cast<const char*>(CurrentSection.Name);

			if (SectionName == ReqestedSectionName)
				return { (ImageBase + CurrentSection.VirtualAddress), CurrentSection.Misc.VirtualSize };
		}

		return { NULL, 0 };
	}

	template<typename CallbackParamType, typename ReturnType = void, typename CallbackType = ReturnType * (*)(CallbackParamType& StateData, const void* SectionBase, const uint32_t SectionSize)>
	inline void* ForEachReadableSectionsInProcess(CallbackType Callback, CallbackParamType& InOutStateData, bool bShouldSkipExecutableSections = true)
	{
		auto [Sections, NumSections] = GetSectionHeader();

		for (int i = 0; i < NumSections; i++)
		{
			const IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

			const std::string SectionName = reinterpret_cast<const char*>(CurrentSection.Name);

			if ((CurrentSection.Characteristics & IMAGE_SCN_MEM_READ) == IMAGE_SCN_MEM_READ)
			{
				if (bShouldExcludeExecutableSections && (CurrentSection.Characteristics & IMAGE_SCN_MEM_EXECUTE) == IMAGE_SCN_MEM_EXECUTE)
					continue;

				if (ReturnType* Result = Callback(InOutStateData, reinterpret_cast<void*>(ImageBase + CurrentSection.VirtualAddress), CurrentSection.Misc.VirtualSize))
				{
					return Result;
				}
			}
		}

		return nullptr;
	}


	template<typename T>
	inline T* FindAlignedValueInRange(const T Value, int32_t Alignment, uintptr_t StartAddress, uint32_t Range)
	{
		constexpr int32_t ElementSize = sizeof(T);

		for (uint32_t i = 0x0; i <= (Range - ElementSize); i += Alignment)
		{
			T* TypedPtr = reinterpret_cast<T*>(StartAddress + i);

			if (*TypedPtr == Value)
				return TypedPtr;
		}

		return nullptr;
	}

	template<typename T>
	inline T* FindAlignedValueInProcessRange(const T InValue, const int32_t InAlignment, bool bShouldSkipExecutableSections = true)
	{
		struct IterateSectionsParams
		{
			const T Value = InValue;
			const int32_t Alignment = InAlignment;
		} Data;

		auto FindValueLambda = [](IterateSectionsParams& Params, const void* SectionBase, const uint32_t SectionSize) -> T*
		{
			return FindAlignedValueInRange<T>(Params.Value, Params.Alignment, reinterpret_cast<uintptr_t>(SectionBase), SectionSize))
		};

		return ForEachReadableSectionsInProcess<IterateSectionsParams, T>(FindValueLambda, Data, bShouldSkipExecutableSections);
	}

	inline void* FindPatternInRange(const std::vector<int>& Signature, const uint8_t* Start, const uintptr_t Range, bool bIsRelative = false, uint32_t Offset = 0, int NumOccurencesToSkip = 0)
	{
		const auto PatternLength = Signature.size();

		for (int i = 0; i < (Range - PatternLength); i++)
		{
			bool bFoundPattern = true;
			int NumOccurencesSkipped = 0;

			for (auto j = 0ul; j < PatternLength; ++j)
			{
				if (Start[i + j] != Signature[j] && Signature[j] != -1)
				{
					bFoundPattern = false;
					break;
				}
			}

			if (!bFoundPattern)
				continue;

			if (NumOccurencesSkipped < NumOccurencesToSkip)
			{
				NumOccurencesSkipped++;
				continue;
			}

			uintptr_t Address = reinterpret_cast<uintptr_t>(Start + i);
			if (bIsRelative)
			{
				if (Offset == -1)
					Offset = PatternLength;

				// Resolve jmp, lea, move, etc. (x86-64)
				Address = ((Address + Offset + 4) + *reinterpret_cast<int32_t*>(Address + Offset));
			}

			return reinterpret_cast<void*>(Address);
		}

		return nullptr;
	}

	inline void* FindPatternInRange(const char* Signature, const uint8_t* Start, uintptr_t Range, bool bRelative = false, uint32_t Offset = 0)
	{
		static auto patternToByte = [](const char* Pattern) -> std::vector<int>
		{
			std::vector<int> Bytes;

			for (auto Current = const_cast<char*>(Pattern); Current != '\0'; ++Current)
			{
				if (*Current == '?')
				{
					++Current;

					// Check next character is also ? to support both single- and double-questionmarks
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

		return FindPatternInRange(patternToByte(Signature), Start, Range, bRelative, Offset);
	}

	inline void* FindPatternInSection(const char* Signature, uint32_t Offset = 0, bool bSearchAllSections = false, uintptr_t StartAddress = 0x0)
	{
		//std::cout << "StartAddr: " << StartAddress << "\n";

		const auto [ImageBase, ImageSize] = GetModuleBaseAndSize();

		uintptr_t SearchStart = ImageBase;
		uintptr_t SearchRange = ImageSize;

		if (!bSearchAllSections)
		{
			const auto [TextSection, TextSize] = GetSectionByName(ImageBase, ".text");

			if (TextSection != 0x0 && TextSize != 0x0)
			{
				SearchStart = TextSection;
				SearchRange = TextSize;
			}
			else
			{
				bSearchAllSections = true;
			}
		}

		const uintptr_t SearchEnd = ImageBase + SearchRange;

		/* If the StartAddress is not default nullptr, and is out of memory-range */
		if (StartAddress != 0x0 && (StartAddress < SearchStart || StartAddress >= SearchEnd))
			return nullptr;

		/* Add a byte to the StartAddress to prevent instantly returning the previous result */
		SearchStart = StartAddress != 0x0 ? (StartAddress + 0x1) : ImageBase;
		SearchRange = StartAddress != 0x0 ? SearchEnd - StartAddress : ImageSize;

		return FindPatternInRange(Signature, reinterpret_cast<uint8_t*>(SearchStart), SearchRange, Offset != 0x0, Offset);
	}
}

