#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <unordered_set>

#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Generators/Generator.h"

static std::unordered_set<int32> RootedClasses;

static void CaptureCurrentClasses()
{
	int32 NewCount = 0;
	for (int32 i = 0; i < ObjectArray::Num(); i++)
	{
		UEObject Obj = ObjectArray::GetByIndex(i);
		if (!Obj || !Obj.IsA(EClassCastFlags::Class)) continue;

		void* Addr = Obj.GetAddress();
		int32 Index = Obj.GetIndex();
		if (!RootedClasses.insert(Index).second) continue;

		uint32& ObjFlags = *reinterpret_cast<uint32*>(static_cast<uint8*>(Addr) + Off::UObject::Flags);
		ObjFlags |= static_cast<uint32>(EObjectFlags::MarkAsRootSet);
		ObjectArray::AddObjectToRoot(Index);
		NewCount++;
	}

	std::cerr << std::format("[+] Captured {} new classes ({} total rooted)\n", NewCount, RootedClasses.size());
	std::cerr << "[*] Load the next level and press F4 again, or press F5 to generate the SDK.\n";
}

static void RunGenerators()
{
	if (Settings::Generator::GameName.empty() && Settings::Generator::GameVersion.empty())
	{
		FString Name;
		FString Version;
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEFunction GetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
		UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");

		Kismet.ProcessEvent(GetGameName, &Name);
		Kismet.ProcessEvent(GetEngineVersion, &Version);

		Settings::Generator::GameName = Name.ToString();
		Settings::Generator::GameVersion = Version.ToString();
	}

	std::cerr << "GameName: " << Settings::Generator::GameName << "\n";
	std::cerr << "GameVersion: " << Settings::Generator::GameVersion << "\n\n";
	std::cerr << "FolderName: " << (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName) << "\n\n";

	Generator::InitInternal();

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();
}

static void GenerateSDK()
{
	if (RootedClasses.empty())
	{
		std::cerr << "[-] No classes captured yet - press F4 first.\n";
		return;
	}

	std::cerr << std::format("[+] Generating SDK from {} rooted classes...\n", RootedClasses.size());
	const auto StartTime = std::chrono::high_resolution_clock::now();

	RunGenerators();

	const auto EndTime = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double, std::milli> DumpTime = EndTime - StartTime;
	std::cerr << std::format("\n\nGenerating SDK took ({:.0f}ms)\n\n\n", DumpTime.count());
}

#if 0
static void LoadAllClasses_Stub()
{
	static UEObject   AssetRegistryHelpers = ObjectArray::FindObject("AssetRegistryHelpers AssetRegistry.Default__AssetRegistryHelpers");
	static UEFunction GetAssetRegistry     = ObjectArray::FindObject<UEFunction>("Function AssetRegistry.AssetRegistryHelpers.GetAssetRegistry", EClassCastFlags::Function);
	static UEFunction GetAllAssets         = ObjectArray::FindObject<UEFunction>("Function AssetRegistry.AssetRegistry.GetAllAssets", EClassCastFlags::Function);
	static UEFunction ConvFunc             = ObjectArray::FindObject<UEFunction>("Function Engine.KismetSystemLibrary.LoadAsset_Blocking", EClassCastFlags::Function);
	static UEClass    KismetClass          = ObjectArray::FindClassFast("KismetSystemLibrary");

	std::cerr << std::format("[{}] AssetRegistryHelpers  : {}\n", AssetRegistryHelpers ? '+' : '-', AssetRegistryHelpers ? AssetRegistryHelpers.GetFullName() : "NOT FOUND");
	std::cerr << std::format("[{}] GetAssetRegistry       : {}\n", GetAssetRegistry     ? '+' : '-', GetAssetRegistry     ? GetAssetRegistry.GetFullName()     : "NOT FOUND");
	std::cerr << std::format("[{}] GetAllAssets           : {}\n", GetAllAssets         ? '+' : '-', GetAllAssets         ? GetAllAssets.GetFullName()         : "NOT FOUND");
	std::cerr << std::format("[{}] LoadAsset_Blocking      : {}\n", ConvFunc             ? '+' : '-', ConvFunc             ? ConvFunc.GetFullName()             : "NOT FOUND");
	std::cerr << std::format("[{}] KismetSystemLibrary    : {}\n", KismetClass          ? '+' : '-', KismetClass          ? KismetClass.GetFullName()          : "NOT FOUND");

	if (!AssetRegistryHelpers || !GetAssetRegistry || !GetAllAssets || !ConvFunc || !KismetClass)
	{
		std::cerr << "[-] LoadAllClasses: One or more required objects not found, aborting!\n";
		return;
	}

	void* AssetRegistryPtr = nullptr;
	AssetRegistryHelpers.ProcessEvent(GetAssetRegistry, &AssetRegistryPtr);

	if (!AssetRegistryPtr)
	{
		std::cerr << "[-] LoadAllClasses: GetAssetRegistry returned null!\n";
		return;
	}

	UEObject AssetRegistry(AssetRegistryPtr);
	std::cerr << std::format("[+] AssetRegistry at 0x{:X}\n", reinterpret_cast<uintptr_t>(AssetRegistryPtr));

	struct
	{
		TArray<uint8> OutAssetData;
		bool bIncludeOnlyOnDiskAssets;
		bool ReturnValue;
	} Params;

	Params.bIncludeOnlyOnDiskAssets = false;
	AssetRegistry.ProcessEvent(GetAllAssets, &Params);

	const int32 NumAssets = Params.OutAssetData.Num();
	std::cerr << std::format("[+] Found {} asset descriptors, force-loading...\n", NumAssets);

	// Query runtime layout of FAssetData and FSoftObjectPath from GObjects reflection
	static UEStruct AssetDataStruct   = ObjectArray::FindStructFast("AssetData");
	static UEStruct SoftObjPathStruct = ObjectArray::FindStructFast("SoftObjectPath");

	if (!AssetDataStruct)
	{
		std::cerr << "[-] LoadAllClasses: AssetData struct not found in GObjects!\n";
		return;
	}

	const int32 AssetDataSize = AssetDataStruct.GetStructSize();
	const int32 FNameSz       = Off::InSDK::Name::FNameSize;

	// FAssetData memory layout starts: FName PackageName, FName PackagePath, FName AssetName
	constexpr int32 PackageNameOff = 0;
	const     int32 AssetNameOff   = 2 * FNameSz; // skip PackageName + PackagePath

	// TSoftObjectPtr<UObject> = TPersistentObjectPtr<FSoftObjectPath>:
	//   FWeakObjectPtr(8) + [TagAtLastTest(4) + pad(4)]? + FSoftObjectPath
	const int32 TagSize      = Settings::Internal::bIsWeakObjectPtrWithoutTag ? 0 : 4;
	const int32 SoftPathSize = SoftObjPathStruct ? SoftObjPathStruct.GetStructSize() : 0x10;
	const int32 SoftPathOff  = (8 + TagSize + 7) & ~7; // align FSoftObjectPath to 8
	const int32 TSoftPtrSize = SoftPathOff + SoftPathSize;
	const int32 RetValOff    = (TSoftPtrSize + static_cast<int32>(sizeof(void*)) - 1) & ~(static_cast<int32>(sizeof(void*)) - 1);
	const int32 ParamBufSize = RetValOff + static_cast<int32>(sizeof(void*));

	// UE5.1+: FSoftObjectPath uses FTopLevelAssetPath {FName PackageName, FName AssetName}
	// UE4/5.0: FSoftObjectPath uses FString AssetPath (or AssetLongPathname)
	const bool bNewSoftPath = [&]() -> bool {
		if (!SoftObjPathStruct) return false;
		const UEProperty AssetPathProp = SoftObjPathStruct.FindMember("AssetPath");
		return AssetPathProp && AssetPathProp.IsA(EClassCastFlags::StructProperty);
	}();

	std::cerr << std::format("[+] Layout: FNameSz={} AssetDataSz={} TagSize={} SoftPathMode={} SoftPathSz={} SoftPathOff={} RetValOff={} ParamBufSz={}\n",
		FNameSz, AssetDataSize, TagSize,
		bNewSoftPath ? "FTopLevelAssetPath" : (SoftObjPathStruct ? "FString(two)" : "FString(legacy)"),
		SoftPathSize, SoftPathOff, RetValOff, ParamBufSize);

	// FAssetData layout (UE5.1+): PackageName[0] PackagePath[8] AssetName[16] AssetClass_deprecated[24] AssetClassPath{PackageName[32],AssetName[40]}
	// The old FName AssetClass at +0x18 is kept for compatibility but is always None/empty in UE5.1+.
	// We filter on AssetClassPath.AssetName (+0x28 = 5*FNameSz) which holds the short class name
	// (e.g. "BlueprintGeneratedClass"). AssetClassPath.PackageName (+0x20 = 4*FNameSz) is the
	// same constant (/Script/Engine) for all standard asset classes and cannot be used to filter.
	// UE4/5.0 used a single FName AssetClass at +0x18 = 3*FNameSz.
	const int32 AssetClassOff = bNewSoftPath ? (5 * FNameSz) : (3 * FNameSz);

	// Scan GObjects for UClass objects whose name ends with "BlueprintGeneratedClass".
	// Collecting their FName CompIdxes lets us filter FAssetData.AssetClassPath cheaply
	// without any string allocation inside the hot loop.
	std::unordered_set<uint32> BGCTypeCompIdxes;
	for (int32 i = 0; i < ObjectArray::Num(); i++)
	{
		const UEObject Obj = ObjectArray::GetByIndex(i);
		if (!Obj || !Obj.IsA(EClassCastFlags::Class)) continue;

		const std::string Name = Obj.GetName();
		if (Name.size() >= 23 && Name.compare(Name.size() - 23, 23, "BlueprintGeneratedClass") == 0)
			BGCTypeCompIdxes.insert(Obj.GetFName().GetCompIdx());
	}
	std::cerr << std::format("[+] Found {} BlueprintGeneratedClass type variant(s) in GObjects\n", BGCTypeCompIdxes.size());

	if (BGCTypeCompIdxes.empty())
	{
		std::cerr << "[-] LoadAllClasses: No BlueprintGeneratedClass types found in GObjects, aborting!\n";
		return;
	}

	UEObject KismetCDO = KismetClass.GetDefaultObject();
	std::cerr << std::format("[{}] KismetCDO at 0x{:X}\n", KismetCDO ? '+' : '-', KismetCDO ? reinterpret_cast<uintptr_t>(KismetCDO.GetAddress()) : 0);

	const uint8* AssetDataArr = Params.OutAssetData.GetDataPtr();

	// If C:/Dumper-7/Whitelist.txt exists, load only the named classes (one per line).
	// If absent, enumerate all Blueprint asset names to LoadLog.txt without loading anything.
	// Lines starting with '#' are treated as comments.
	std::unordered_set<std::wstring> Whitelist;
	{
		std::ifstream WLFile("C:/Dumper-7/Whitelist.txt");
		if (WLFile)
		{
			std::string Line;
			while (std::getline(WLFile, Line))
			{
				while (!Line.empty() && (Line.back() == '\r' || Line.back() == ' ' || Line.back() == '\t'))
					Line.pop_back();
				if (!Line.empty() && Line[0] != '#')
					Whitelist.insert(std::wstring(Line.begin(), Line.end()));
			}
			std::cerr << std::format("[+] Whitelist.txt: {} entries\n", Whitelist.size());
		}
	}

	std::ofstream LoadLog("C:/Dumper-7/LoadLog.txt");

	if (Whitelist.empty())
	{
		// Dump mode: write all Blueprint asset names to LoadLog.txt, do not load anything.
		std::cerr << "[*] No Whitelist.txt found \xe2\x80\x94 dumping all Blueprint names to C:/Dumper-7/LoadLog.txt\n";
		std::cerr << "[*] Add class names (one per line) to C:/Dumper-7/Whitelist.txt and re-inject to load them.\n";

		if (LoadLog)
		{
			LoadLog << std::format("Blueprint asset list ({} total). Add class names to C:/Dumper-7/Whitelist.txt to load them.\n\n",
				NumAssets);

			int32 Count = 0;
			for (int32 i = 0; i < NumAssets; i++)
			{
				const uint8* AssetData = AssetDataArr + static_cast<int64>(i) * AssetDataSize;
				const uint32 AssetClassCompIdx = *reinterpret_cast<const uint32*>(AssetData + AssetClassOff);
				if (!BGCTypeCompIdxes.count(AssetClassCompIdx)) continue;

				const std::wstring PkgW   = FName(AssetData + PackageNameOff).ToWString();
				const std::wstring NameW  = FName(AssetData + AssetNameOff).ToWString();
				const std::wstring ClassW = FName(AssetData + AssetClassOff).ToWString();

				LoadLog << std::format("[{:05d}] i={:6d}  {}.{}  [{}]\n",
					++Count, i,
					std::string(PkgW.begin(),   PkgW.end()),
					std::string(NameW.begin(),  NameW.end()),
					std::string(ClassW.begin(), ClassW.end()));
			}
			LoadLog << std::format("\n{} Blueprint assets total.\n", Count);
			LoadLog.flush();
		}
		std::cerr << "[+] Dump complete \xe2\x80\x94 see C:/Dumper-7/LoadLog.txt\n";
		return;
	}

	// Whitelist load mode: load only the named classes.
	// Skip classes already resident in GObjects to avoid redundant loads.
	std::unordered_set<uint32> AlreadyLoadedCompIdxes;
	for (int32 j = 0; j < ObjectArray::Num(); j++)
	{
		const UEObject Obj = ObjectArray::GetByIndex(j);
		if (!Obj || !Obj.IsA(EClassCastFlags::Class)) continue;
		AlreadyLoadedCompIdxes.insert(Obj.GetFName().GetCompIdx());
	}
	std::cerr << std::format("[+] {} UClass already in GObjects\n", AlreadyLoadedCompIdxes.size());

	std::vector<uint8> ParamBuf(ParamBufSize, 0);
	int32 NumLoaded  = 0;
	int32 NumCrashed = 0;
	int32 NumSkipped = 0;

	if (LoadLog)
		LoadLog << std::format("Whitelist load \xe2\x80\x94 {} entries  ({} UClass already in GObjects)\n\n",
			Whitelist.size(), AlreadyLoadedCompIdxes.size());

	for (int32 i = 0; i < NumAssets; i++)
	{
		const uint8* AssetData = AssetDataArr + static_cast<int64>(i) * AssetDataSize;
		const uint32 AssetClassCompIdx = *reinterpret_cast<const uint32*>(AssetData + AssetClassOff);
		if (!BGCTypeCompIdxes.count(AssetClassCompIdx)) continue;

		const uint32 NameCompIdx = *reinterpret_cast<const uint32*>(AssetData + AssetNameOff);
		if (AlreadyLoadedCompIdxes.count(NameCompIdx)) { NumSkipped++; continue; }

		const std::wstring NameW = FName(AssetData + AssetNameOff).ToWString();
		if (!Whitelist.count(NameW)) continue;

		const std::wstring PkgW   = FName(AssetData + PackageNameOff).ToWString();
		const std::wstring ClassW = FName(AssetData + AssetClassOff).ToWString();

		if (LoadLog)
		{
			LoadLog << std::format("{}.{} [{}] -> ",
				std::string(PkgW.begin(),   PkgW.end()),
				std::string(NameW.begin(),  NameW.end()),
				std::string(ClassW.begin(), ClassW.end()));
			LoadLog.flush();
		}

		memset(ParamBuf.data(), 0, ParamBufSize);
		*reinterpret_cast<int32*>(ParamBuf.data()) = -1;
		uint8* SoftPath = ParamBuf.data() + SoftPathOff;

		if (bNewSoftPath)
		{
			memcpy(SoftPath,           AssetData + PackageNameOff, FNameSz);
			memcpy(SoftPath + FNameSz, AssetData + AssetNameOff,   FNameSz);
		}
		else
		{
			std::wstring PathW = PkgW + L"." + NameW;
			FString PathFStr(PathW.data(), static_cast<int32>(PathW.size() + 1), static_cast<int32>(PathW.size() + 1));
			memcpy(SoftPath, &PathFStr, sizeof(FString));
		}

		const bool bOk = SafeProcessEvent(KismetCDO, ConvFunc, ParamBuf.data());
		Sleep(0);

		void* RetVal = bOk ? *reinterpret_cast<void**>(ParamBuf.data() + RetValOff) : nullptr;

		if (!bOk)        NumCrashed++;
		else if (RetVal) NumLoaded++;

		if (LoadLog)
		{
			if (!bOk)        LoadLog << "EXCEPTION\n";
			else if (RetVal) LoadLog << std::format("OK 0x{:X}\n", reinterpret_cast<uintptr_t>(RetVal));
			else             LoadLog << "null\n";
			LoadLog.flush();
		}
	}

	std::cerr << std::format("[+] Loaded {}/{} whitelisted Blueprint classes ({} crashed, {} already-in-GObjects skipped)\n",
		NumLoaded, Whitelist.size(), NumCrashed, NumSkipped);
	if (LoadLog)
		LoadLog << std::format("\nDone. loaded={} crashed={} already_in_gobjects_skipped={}\n",
			NumLoaded, NumCrashed, NumSkipped);
}
#endif

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	std::cerr << "Started Generation [Dumper-7]!\n";

	Settings::Config::Load();

	if (Settings::Config::SleepTimeout > 0)
	{
		std::cerr << "Sleeping for " << Settings::Config::SleepTimeout << "ms...\n";
		Sleep(Settings::Config::SleepTimeout);
	}

	Generator::InitEngineCore();

	if (Settings::Config::bMultiLevelCapture)
	{
		std::cerr << "\n[Dumper-7] Multi-level capture mode:\n";
		std::cerr << "  F4 = Capture current classes (press in each level)\n";
		std::cerr << "  F5 = Generate SDK from all captured classes\n";
		std::cerr << "  F6 = Eject\n\n";

		while (true)
		{
			if (GetAsyncKeyState(VK_F4) & 1)
				CaptureCurrentClasses();

			if (GetAsyncKeyState(VK_F5) & 1)
				GenerateSDK();

			if (GetAsyncKeyState(VK_F6) & 1)
			{
				fclose(stderr);
				if (Dummy) fclose(Dummy);
				FreeConsole();
				FreeLibraryAndExitThread(Module, 0);
			}

			Sleep(100);
		}
	}
	else
	{
		std::cerr << "\n[Dumper-7] Controls:\n";
		std::cerr << "  F5 = Generate SDK\n";
		std::cerr << "  F6 = Eject\n\n";

		while (true)
		{
			if (GetAsyncKeyState(VK_F5) & 1)
			{
				const auto StartTime = std::chrono::high_resolution_clock::now();

				RunGenerators();

				const auto EndTime = std::chrono::high_resolution_clock::now();
				const std::chrono::duration<double, std::milli> DumpTime = EndTime - StartTime;
				std::cerr << std::format("\n\nGenerating SDK took ({:.0f}ms)\n\n\n", DumpTime.count());
			}

			if (GetAsyncKeyState(VK_F6) & 1)
			{
				fclose(stderr);
				if (Dummy) fclose(Dummy);
				FreeConsole();
				FreeLibraryAndExitThread(Module, 0);
			}

			Sleep(100);
		}
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
		break;
	}

	return TRUE;
}
