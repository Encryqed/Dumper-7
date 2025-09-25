#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Generators/Generator.h"


// Using an atomic bool allows us to reliably wait on another thread without race conditions
std::atomic<bool> dumpStarted = false;

// Use the actual value for VK scancodes, e.g. 0x77 = F8
void ListenerThread(int key)
{
	while (!dumpStarted)
	{
		dumpStarted = GetAsyncKeyState(key) & 0x8000;
	}
	return;
}


static void InitSettings() 
{
	Settings::Config::Load();

	static auto t_0 = std::chrono::high_resolution_clock::now();

	if (Settings::Config::DumpKey == 0)
	{
		// If neither option is set we can resume execution on the main thread
		if (Settings::Config::SleepTimeout == 0)
			return; 

		// If a timeout is set without a dumpkey we will just synchronously wait and then dump
		std::cerr << "Sleeping for " << Settings::Config::SleepTimeout << (Settings::Config::SleepTimeout < 1000) ? "seconds...\n" : "ms...\n";
		Sleep(Settings::Config::SleepTimeout);
		return;
	}
	
	// If we have a dump key set we will spawn a listener thread and wait for it to signal us
	std::thread listenerThread(&ListenerThread, Settings::Config::DumpKey);
	listenerThread.detach();

	auto t_x = std::chrono::high_resolution_clock::now();

	while (!dumpStarted)
	{
		// And finally in the case that both a dump key and timeout are set we will continue checking back on this thread 
		if (Settings::Config::SleepTimeout > 0) 
		{
			t_x = std::chrono::high_resolution_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_x - t_0).count();
			if (diff >= Settings::Config::SleepTimeout) 
			{
				// If the timeout is reached before the key is pressed we can return to our main thread
				dumpStarted = true;
				std::cerr << "Timeout reached, dumping SDK...\n";
				return;
			}
		}
		// Loop body
		Sleep(100);
	}
	// Key was pressed, return to main thread
	return;
}


enum class EFortToastType : uint8
{
	Default = 0,
	Subdued = 1,
	Impactful = 2,
	EFortToastType_MAX = 3,
};


DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	// Set the module path so we can check for possible config locations
	char moduleBuf[256] = {};
	// Use our existing handle without having to enumerate or assume our DLL name
	GetModuleFileNameA(Module, moduleBuf, sizeof(moduleBuf));
	Settings::Config::ModulePath = moduleBuf;
	
	std::cerr << "Loaded Dumper-7 from " << Settings::Config::ModulePath << "\n";
	
	// Configure settings and wait for dump key or timeout if set
	InitSettings();
	
	auto t_1 = std::chrono::high_resolution_clock::now();

	std::cerr << "Started Generation [Dumper-7]!\n";

	Generator::InitEngineCore();
	Generator::InitInternal();

	if (Settings::Generator::GameName.empty() && Settings::Generator::GameVersion.empty())
	{
		// Only Possible in Main()
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

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();

	auto t_C = std::chrono::high_resolution_clock::now();

	auto ms_int_ = std::chrono::duration_cast<std::chrono::milliseconds>(t_C - t_1);
	std::chrono::duration<double, std::milli> ms_double_ = t_C - t_1;

	std::cerr << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	while (true)
	{
		// If a dump key was set we can reuse that key, otherwise default to F6
		if (GetAsyncKeyState(Settings::Config::DumpKey != 0 ? Settings::Config::DumpKey : VK_F6) & 1)
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
