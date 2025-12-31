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

std::atomic<bool> dumpStarted = false;

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

/*
	this block makes it so the game and main dump thread 
	doesn't pause when clicking and highlighting text in the console
	if this is considered a useful/desirable behavior remove this
*/
	auto hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE;   
	SetConsoleMode(hStdin, mode);

	// using shorthands to avoid wrapping some longer lines
	using namespace std::chrono;
	namespace cfg = Settings::Config;
	auto t_1 = high_resolution_clock::now();
	
	cfg::Load();
	
	if (cfg::DumpKey != 0)
	{
		// we must use a detached thread to wait for the keypress otherwise we just run or unload if the first check fails
		// using a lambda improves readability but requires more care to avoid an std::terminate being called
		// the thread has to be detached or joined before it goes out of scope and it can't have any dangling resources
		// therefore we avoid capturing anything by reference and pass direct values for the minimal necessary data 
		std::thread listenerThread([Key = cfg::DumpKey, st = t_1, SleepTimeout = cfg::SleepTimeout]() 
		{
			while (true)
			{
				// 0x8000 means that the key has to be currently held so we need relatively frequent updates
				if (GetAsyncKeyState(Key) & 0x8000) break;
				// If sleep timeout is 0 we don't have to evaluate past the first expression
				// If std::chrono isn't abbreviated we have to wrap this line or evaluate the time diff in a temp variable
				if (0 < SleepTimeout && duration_cast<milliseconds>(high_resolution_clock::now() - st).count() > SleepTimeout)
				{
					std::cerr << "Sleep Timeout exceeded, proceeding with dump...\n";
					break;
				}
				Sleep(50); // 50ms should be a good balance to get keypresses without any cpu strain
			}
			dumpStarted = true; // tell the main thread loop to proceed. This thread will cleanup on its own
		});
		listenerThread.detach();
		// we have to do something to keep our main thread going and block the dump from starting
		while (!dumpStarted) Sleep(50);	
	}
	else // no dump key set, default behavior, sleep timeout is printed during config load now
	{
		Sleep(cfg::SleepTimeout); // Sleep(0) is fine here it will just yield the thread
	}

	// move the started generation message after any timeout
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
	
	// calculate time inline. only possible to fit without wrapping with std::chrono abbreviated, either way better to calc inline
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
