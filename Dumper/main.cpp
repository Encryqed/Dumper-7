#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include "Settings.h"
#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"
#include "Generators/Generator.h"

enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};

// ── Vectored exception handler ─────────────────────────────────────────────
// Runs before the default crash handler, giving us a chance to flush the log.
static LONG WINAPI VEH_CrashHandler(EXCEPTION_POINTERS* pEx)
{
	// Ignore non-fatal exceptions like C++ exceptions in-flight or STATUS_BREAKPOINT
	DWORD code = pEx->ExceptionRecord->ExceptionCode;
	if (code == 0xE06D7363 /*C++ exception*/ || code == EXCEPTION_BREAKPOINT)
		return EXCEPTION_CONTINUE_SEARCH;

	Settings::LogFmt("\n!!! CRASH: exception code=0x%08X at address=0x%p",
		code, pEx->ExceptionRecord->ExceptionAddress);

	// Log parameters (helpful for access violations: [0]=read/write flag, [1]=bad address)
	for (DWORD i = 0; i < pEx->ExceptionRecord->NumberParameters && i < 4; i++)
		Settings::LogFmt("    ExceptionInformation[%lu] = 0x%llX", i, (unsigned long long)pEx->ExceptionRecord->ExceptionInformation[i]);

	return EXCEPTION_CONTINUE_SEARCH; // let the normal crash dialog appear
}

// ── Main ───────────────────────────────────────────────────────────────────
DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	std::cerr.clear();
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	Settings::Log("Initializing [Dumper-7]");

	Settings::Config::Load();
	Settings::Log("[Config] Load() done");

	// Install exception handler if enabled in config
	PVOID vehHandle = nullptr;
	if (Settings::Debug::bEnableVectoredCrashHandler)
	{
		vehHandle = AddVectoredExceptionHandler(1 /*first in chain*/, VEH_CrashHandler);
		Settings::Log("[Debug] Vectored exception handler installed");
	}

	Settings::Config::DelayDumperStart();
	Settings::Log("[Config] DelayDumperStart() done");

	Settings::Log("Started Generation [Dumper-7]!");
	auto DumpStartTime = std::chrono::high_resolution_clock::now();

	// ── InitEngineCore ────────────────────────────────────────────────────
	Settings::Log("[Step] >>> Generator::InitEngineCore()");
	Generator::InitEngineCore();
	Settings::Log("[Step] <<< Generator::InitEngineCore() complete");

	// ── InitInternal ──────────────────────────────────────────────────────
	Settings::Log("[Step] >>> Generator::InitInternal()");
	Generator::InitInternal();
	Settings::Log("[Step] <<< Generator::InitInternal() complete");

	// ── Game name / version ───────────────────────────────────────────────
	if (Settings::Generator::GameName.empty() && Settings::Generator::GameVersion.empty())
	{
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		if (Kismet)
		{
			UEFunction GetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
			UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");

			if (GetGameName && GetEngineVersion)
			{
				FString Name;
				FString Version;
				__try
				{
					Kismet.ProcessEvent(GetGameName, &Name);
					Kismet.ProcessEvent(GetEngineVersion, &Version);

					Settings::Generator::GameName = Name.ToString();
					Settings::Generator::GameVersion = Version.ToString();
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					Settings::Log("[Warning] Crashed while attempting to auto-detect GameName and GameVersion via KismetSystemLibrary ProcessEvent.");
				}
			}
		}

		if (Settings::Generator::GameName.empty())
			Settings::Generator::GameName = "UE-Game";
		if (Settings::Generator::GameVersion.empty())
			Settings::Generator::GameVersion = "UnknownVersion";
	}

	Settings::LogFmt("GameName: %s", Settings::Generator::GameName.c_str());
	Settings::LogFmt("GameVersion: %s", Settings::Generator::GameVersion.c_str());
	Settings::LogFmt("FolderName: %s", (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName).c_str());

	// ── Generate ──────────────────────────────────────────────────────────
	Settings::Log("[Step] >>> Generator::Generate<CppGenerator>()");
	Generator::Generate<CppGenerator>();
	Settings::Log("[Step] <<< CppGenerator done");

	Settings::Log("[Step] >>> Generator::Generate<MappingGenerator>()");
	Generator::Generate<MappingGenerator>();
	Settings::Log("[Step] <<< MappingGenerator done");

	Settings::Log("[Step] >>> Generator::Generate<IDAMappingGenerator>()");
	Generator::Generate<IDAMappingGenerator>();
	Settings::Log("[Step] <<< IDAMappingGenerator done");

	Settings::Log("[Step] >>> Generator::Generate<DumpspaceGenerator>()");
	Generator::Generate<DumpspaceGenerator>();
	Settings::Log("[Step] <<< DumpspaceGenerator done");

	auto DumpFinishTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> DumpTime = DumpFinishTime - DumpStartTime;
	Settings::LogFmt("Generating SDK took (%.1fms)", DumpTime.count());

	if (Settings::Debug::bExecuteSDKTestScript)
	{
		Settings::Log("[Step] Executing SDK compilation test script...");
		CppGenerator::ExecuteSDKCompilationTestScript();
	}

	Settings::Log("=== Generation complete. Press F6 to unload. ===");

	if (vehHandle) RemoveVectoredExceptionHandler(vehHandle);

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stderr);
			if (Dummy) fclose(Dummy);
			FreeConsole();
			FreeLibraryAndExitThread(Module, 0);
		}
		Sleep(100);
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