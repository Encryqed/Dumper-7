#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <atomic>

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

// signal for our keylistener. must be declared at file scope to be readable in the lambda
std::atomic<bool> dumpStarted = false;

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	freopen_s(&Dummy, "CONIN$", "r", stdin);


// clicking and highlighting text in the system console pauses execution of game and dump
// this can be disabled by uncommenting the block below. left as an option for developers
/*	
	auto hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE;   
	SetConsoleMode(hStdin, mode);
*/
	// using std::chrono and aliasing cfg solely to avoid wrapping some long statements
	using namespace std::chrono;
	namespace cfg = Settings::Config;
	auto t_1 = high_resolution_clock::now();
	
	cfg::Load();
	
	if (cfg::DumpKey != 0)
	{
		// crucial we capture needed data by value. references will lead to dangling resources and crash
		std::thread listenerThread([Key = cfg::DumpKey, st = t_1, SleepTimeout = cfg::SleepTimeout]() 
		{
			while (true) // only allow manually breaking out of loop. detached thread can cleanup on early close
			{	
				// check if key is currently pressed
				if (GetAsyncKeyState(Key) & 0x8000) break; 
				// if SleepTimeout is 0 we only eval the first statement 
				if (0 < SleepTimeout && duration_cast<milliseconds>(high_resolution_clock::now() - st).count() > SleepTimeout)
				{
					std::cerr << "Sleep Timeout exceeded, proceeding with dump...\n";
					break;
				}
				Sleep(50); // 50ms should be a good balance to get keypresses without any cpu strain
			}
			dumpStarted = true; // tell the main thread loop to proceed. This thread will cleanup on its own
		});
		listenerThread.detach(); // detach immediately while lambda is in scope to avoid std::terminate
		// spin lock main thread otherwise we end up unloading or proceeding to dump early
		while (!dumpStarted) Sleep(50);	
	}
	else // no dump key set, sleep timeout will occur if set
	{
		Sleep(cfg::SleepTimeout); // Sleep(0) simply yields the thread without delay so we don't even have to check the timeout
	}

	// announce start only after we actually start
	std::cerr << "Started Generation [Dumper-7]!\n";
	// reuse the timepoint var we set for the dumpkey
	t_1 = high_resolution_clock::now();

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

	std::cerr << "FolderName: " << (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName) << "\n\n";

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();
	
	// calculate time inline. without using namespace std::chrono this can still be a single statement but has to be wrapped
	std::cerr << "\n\nGenerating SDK took (" << duration_cast<milliseconds>(high_resolution_clock::now() - t_1).count() << "ms)\n\n\n";
	std::cerr << "\n\nPress F6 to unload";
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
