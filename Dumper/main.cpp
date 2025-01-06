#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include "CppGenerator.h"
#include "MappingGenerator.h"
#include "IDAMappingGenerator.h"
#include "DumpspaceGenerator.h"

#include "StructManager.h"
#include "EnumManager.h"

#include "Generator.h"


#include "UnicodeNames.h"

enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};

int MainThread()
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	auto t_1 = std::chrono::high_resolution_clock::now();

	std::cout << "Started Generation [Dumper-7]!\n";

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

	std::cout << "GameName: " << Settings::Generator::GameName << "\n";
	std::cout << "GameVersion: " << Settings::Generator::GameVersion << "\n\n";

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();


	auto t_C = std::chrono::high_resolution_clock::now();

	auto ms_int_ = std::chrono::duration_cast<std::chrono::milliseconds>(t_C - t_1);
	std::chrono::duration<double, std::milli> ms_double_ = t_C - t_1;

	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		MainThread();
		break;
	}

	return TRUE;
}