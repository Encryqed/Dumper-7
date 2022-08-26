#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "Enums.h"
#include "Generator.h"
#include "Utils.h"
#include "OffsetFinder.h"
#include "Offsets.h"
#include "Package.h"

enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};


DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	auto t_1 = high_resolution_clock::now();
	
	std::cout << "Started Generation [Dumper-7]!\n";
	
	Generator::Init();

	if (Settings::GameName.empty())
	{
		//Only Possible in Main()
		FString Version;
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");

		Kismet.ProcessEvent(GetEngineVersion, &Version);

		Settings::GameName = Version.ToString();
	}

	std::cout << "GameName: " << Settings::GameName << "\n\n";


	Generator::GenerateSDK();
	
	
	auto t_2 = high_resolution_clock::now();
	
	auto ms_int_ = duration_cast<milliseconds>(t_2 - t_1);
	duration<double, std::milli> ms_double_ = t_2 - t_1;
	
	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stdout);
			fclose(Dummy);
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