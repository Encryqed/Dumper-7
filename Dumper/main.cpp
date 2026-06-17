#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <atomic>
#include <filesystem>
#include <streambuf>
#include <vector>
#include <utility>
#include <algorithm>

#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Generators/Generator.h"

#include "Unreal/ObjectArray.h"
#include "OffsetFinder/Offsets.h"
#include "Platform.h"

enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};

/* ---- File logging: mirror std::cerr to a log file so the latest log survives a game crash ---- */

namespace
{
	/* Fans every byte out to two streambufs (console + file). */
	class TeeStreamBuf : public std::streambuf
	{
		std::streambuf* First;
		std::streambuf* Second;

	public:
		TeeStreamBuf(std::streambuf* InFirst, std::streambuf* InSecond)
			: First(InFirst), Second(InSecond) {}

	protected:
		int overflow(int Ch) override
		{
			if (Ch == traits_type::eof())
				return traits_type::not_eof(Ch);

			const int R1 = First  ? First->sputc(static_cast<char>(Ch))  : Ch;
			const int R2 = Second ? Second->sputc(static_cast<char>(Ch)) : Ch;

			return (R1 == traits_type::eof() || R2 == traits_type::eof()) ? traits_type::eof() : Ch;
		}

		int sync() override
		{
			const int R1 = First  ? First->pubsync()  : 0;
			const int R2 = Second ? Second->pubsync() : 0;
			return (R1 == 0 && R2 == 0) ? 0 : -1;
		}
	};

	std::ofstream   g_LogFile;
	std::streambuf* g_OriginalCerrBuf = nullptr;
}

/* std::cerr is unit-buffered, so each insertion is flushed straight through the tee to the file -
   even an access-violation mid-dump leaves a complete log on disk. */
static void InitFileLogging()
{
	std::error_code Ec;
	std::filesystem::create_directories(Settings::Generator::SDKGenerationPath, Ec);

	const std::string LogPath = std::string(Settings::Generator::SDKGenerationPath) + "/Dumper-7.log";

	g_LogFile.open(LogPath, std::ios::out | std::ios::trunc);
	if (!g_LogFile.is_open())
	{
		std::cerr << "[Dumper-7] WARNING: could not open log file '" << LogPath << "' - console-only logging.\n";
		return;
	}

	static TeeStreamBuf Tee(std::cerr.rdbuf(), g_LogFile.rdbuf());
	g_OriginalCerrBuf = std::cerr.rdbuf(&Tee);

	std::cerr << "[Dumper-7] Logging to '" << LogPath << "'.\n";
}

static void ShutdownFileLogging()
{
	if (g_OriginalCerrBuf)
	{
		std::cerr.rdbuf(g_OriginalCerrBuf);
		g_OriginalCerrBuf = nullptr;
	}

	if (g_LogFile.is_open())
	{
		g_LogFile.flush();
		g_LogFile.close();
	}
}

/*
 * Runs the actual SDK generation. Iterates the entire GObjects array (via the managers) and reads
 * object/property data throughout, so it must run while GObjects is stable. When the game-thread
 * Tick hook is installed it is invoked from inside that hook (game thread parked → GC can't run);
 * otherwise it falls back to running on the dumper's own thread (legacy behavior).
 */
static void RunFullDump()
{
	Generator::InitInternal();

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

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();
}

/* ---- Game-thread synchronization via a UGameEngine::Tick vtable hook ---- */

using TickFnType = void(*)(void* This, float DeltaSeconds, bool bIdleMode);

static TickFnType        g_OriginalTick     = nullptr; // resolved UGameEngine::Tick, used for pass-through calls
static int32             g_TickIdx          = -1;
static std::vector<std::pair<void**, void*>> g_PatchedSlots; // {slot address, original value} for restore
static HANDLE            g_DumpStartedEvent = nullptr; // set when the hook claims the dump
static HANDLE            g_DumpDoneEvent    = nullptr; // set when RunFullDump() returns
static std::atomic<bool> g_DumpClaimed{ false };       // exactly one of {hook, watchdog} runs the dump

static bool WriteVTableSlot(void** Slot, void* Value)
{
	DWORD OldProtect = 0;
	if (!VirtualProtect(Slot, sizeof(void*), PAGE_READWRITE, &OldProtect))
		return false;

	*Slot = Value;

	DWORD Ignored = 0;
	VirtualProtect(Slot, sizeof(void*), OldProtect, &Ignored);
	return true;
}

static void RestoreTickHook()
{
	for (const auto& [Slot, Original] : g_PatchedSlots)
		WriteVTableSlot(Slot, Original);

	g_PatchedSlots.clear();
}

static void TickHook(void* This, float DeltaSeconds, bool bIdleMode)
{
	/* Only the first invocation runs the dump; any other call (watchdog already claimed, or a second
	   engine instance ticking) passes straight through to the real Tick. Pass-through never touches
	   g_PatchedSlots, so it can't race the watchdog's RestoreTickHook(). */
	if (g_DumpClaimed.exchange(true))
		return g_OriginalTick(This, DeltaSeconds, bIdleMode);

	if (g_DumpStartedEvent)
		SetEvent(g_DumpStartedEvent);

	/* Put every patched vtable back before doing heavy work, so we only ever park one frame. */
	RestoreTickHook();

	std::cerr << "[Dumper-7] Game thread parked inside UGameEngine::Tick - running dump...\n";
	RunFullDump();

	if (g_DumpDoneEvent)
		SetEvent(g_DumpDoneEvent);

	/* This' slot is restored now, so call the real Tick through the (restored) vtable - this honors a
	   per-instance override if one exists - and the frame completes normally. */
	const TickFnType RealTick = reinterpret_cast<TickFnType>((*reinterpret_cast<void** const*>(This))[g_TickIdx]);
	RealTick(This, DeltaSeconds, bIdleMode);
}

/* Patch the Tick slot in the vtable of every live (non-CDO) UGameEngine-derived object. We can't tell
   which instance the engine loop actually ticks - a UGameEngine subclass gets its own vtable array, so
   patching just one instance (as before) can miss the real GEngine - so hook them all and let the first
   one that ticks claim the dump. */
static bool InstallTickHook()
{
	if (!Settings::Internal::bHasGameEngineTick || Off::InSDK::Engine::UGameEngineTickIndex < 0)
	{
		std::cerr << "[Dumper-7] UGameEngine::Tick not resolved - cannot sync to game thread.\n";
		return false;
	}

	UEClass EngineClass = ObjectArray::FindClassFast("GameEngine");
	if (!EngineClass)
	{
		std::cerr << "[Dumper-7] GameEngine class not found - cannot sync to game thread.\n";
		return false;
	}

	g_TickIdx      = Off::InSDK::Engine::UGameEngineTickIndex;
	g_OriginalTick = reinterpret_cast<TickFnType>(Platform::GetModuleBase() + Off::InSDK::Engine::UGameEngineTickOffset);

	const UEObject CDO = EngineClass.GetDefaultObject();

	std::vector<void**> SeenVtables;
	int32 NumInstances = 0;

	for (UEObject Obj : ObjectArray())
	{
		if (!Obj || Obj == CDO)
			continue;

		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject))
			continue;

		if (!Obj.IsA(EngineClass))
			continue;

		++NumInstances;

		void** const Vtbl = *reinterpret_cast<void** const*>(Obj.GetAddress());
		if (!Vtbl || Platform::IsBadReadPtr(Vtbl))
			continue;

		/* Distinct instances of the same class share one vtable - patch each vtable only once. */
		if (std::find(SeenVtables.begin(), SeenVtables.end(), Vtbl) != SeenVtables.end())
			continue;

		SeenVtables.push_back(Vtbl);

		void** const Slot     = &Vtbl[g_TickIdx];
		void*  const Original = Vtbl[g_TickIdx];

		if (!Original || Platform::IsBadReadPtr(Original))
			continue;

		if (WriteVTableSlot(Slot, reinterpret_cast<void*>(&TickHook)))
		{
			g_PatchedSlots.emplace_back(Slot, Original);

			const bool bOverride = reinterpret_cast<uintptr_t>(Original) != reinterpret_cast<uintptr_t>(g_OriginalTick);
			std::cerr << std::format("[Dumper-7] Hooked Tick in engine vtable 0x{:X} (slot[{}] was 0x{:X}){}\n",
				reinterpret_cast<uintptr_t>(Vtbl), g_TickIdx, reinterpret_cast<uintptr_t>(Original),
				bOverride ? " [override]" : "");
		}
	}

	if (g_PatchedSlots.empty())
	{
		std::cerr << std::format("[Dumper-7] No engine vtables hooked ({} instance(s) seen) - cannot sync to game thread.\n", NumInstances);
		return false;
	}

	std::cerr << std::format("[Dumper-7] Installed Tick hook on {} engine vtable(s) at slot[{}] ({} instance(s) seen).\n",
		g_PatchedSlots.size(), g_TickIdx, NumInstances);
	return true;
}

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	InitFileLogging();

	std::cerr << "Started Generation [Dumper-7]!\n";

	Settings::Config::Load();

	if (Settings::Config::SleepTimeout > 0)
	{
		std::cerr << "Sleeping for " << Settings::Config::SleepTimeout << "ms...\n";
		Sleep(Settings::Config::SleepTimeout);
	}

	auto DumpStartTime = std::chrono::high_resolution_clock::now();

	/* Offset finding must run first: it resolves the Tick offset/index we hook with. Its GObjects
	   scans are short; the heavy full-array work happens later inside RunFullDump (GC-protected). */
	Generator::InitEngineCore();

	g_DumpStartedEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	g_DumpDoneEvent    = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	if (InstallTickHook())
	{
		std::cerr << "[Dumper-7] Waiting for the game thread to pick up the dump...\n";

		/* Wait for the first tick to claim the dump. If the engine doesn't tick within the grace
		   period, fall back to an off-thread dump (race-safe via g_DumpClaimed). */
		constexpr DWORD FirstTickGraceMs = 20000;
		if (WaitForSingleObject(g_DumpStartedEvent, FirstTickGraceMs) == WAIT_TIMEOUT
			&& !g_DumpClaimed.exchange(true))
		{
			std::cerr << std::dec << "[Dumper-7] Game thread did not tick within " << FirstTickGraceMs
				<< "ms - restoring hook and dumping off-thread.\n";
			RestoreTickHook();
			RunFullDump();
		}
		else
		{
			/* Hook claimed it; wait for the (potentially long) dump to finish on the game thread. */
			WaitForSingleObject(g_DumpDoneEvent, INFINITE);
		}
	}
	else
	{
		std::cerr << "[Dumper-7] Running dump off-thread (no game-thread sync).\n";
		RunFullDump();
	}

	auto DumpFinishTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> DumpTime = DumpFinishTime - DumpStartTime;

	std::cerr << "\n\nGenerating SDK took (" << DumpTime.count() << "ms)\n\n\n";

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			ShutdownFileLogging();

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
