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
	
	ObjectArray::Init();
	FName::Init();
	Off::Init();

	ObjectArray::DumpObjects();

	// this stuff here just for debug reasons 
	//UEObject Vec4Object = ObjectArray::FindObject("ScriptStruct CoreUObject.Vector4");
	/*UEObject Vec4Object = ObjectArray::FindObject("Class Engine.PlayerController");

	std::cout << Vec4Object.Cast<UEStruct>().HasFMembers() << "\n" << std::endl;

	std::cout << Vec4Object.GetFullName() << " has Members!\n";
		
	for (UEFField Field = Vec4Object.Cast<UEStruct>().GetChildProperties(); Field; Field = Field.GetNext())
	{
		if (Field.IsOwnerUObject())
		{
			std::cout << "->    " << Field.GetClass().GetName() << "." << Field.GetName() << std::endl;
		}
	}*/

	/*Generator::Init();

	if (Settings::GameName.empty() && Settings::GameVersion.empty())
	{
		//Only Possible in Main()
		FString Name;
		FString Version;
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEFunction GetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
		UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");
			
		Kismet.ProcessEvent(GetGameName, &Name);
		Kismet.ProcessEvent(GetEngineVersion, &Version);
	
		Settings::GameName = Name.ToString();
		Settings::GameVersion = Version.ToString();
	}
	
	std::cout << "GameName: " << Settings::GameName << "\n";
	std::cout << "GameVersion: " << Settings::GameVersion << "\n\n";

	Generator::GenerateSDK();
	

	auto t_C = high_resolution_clock::now();
	
	auto ms_int_ = duration_cast<milliseconds>(t_C - t_1);
	duration<double, std::milli> ms_double_ = t_C - t_1;
	
	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";*/

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