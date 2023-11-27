#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "Generator.h"
#include "CppGenerator.h"

#include "HashStringTableTest.h"
#include "GeneratorRewriteTest.h"
#include "StructManagerTest.h"
#include "UniqueNameTest.h"

#include "GeneratorRewrite.h"

//#include "CppGeneratorTest.h"
//#include "StringTableTest.h"

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
	//GeneratorRewrite::InitEngineCore();

	if (Settings::GameName.empty() && Settings::GameVersion.empty())
	{
		// Only Possible in Main()
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

	//CppGeneratorTest::TestAll();

	//StringTableTest::TestAll();
	//HashStringTableTest::TestAll();
	//HashStringTableTest::TestUniqueMemberNames();
	//HashStringTableTest::TestUniqueStructNames();
	//
	//GeneratorRewriteTest::TestAll();
	//
	//StructManagerTest::TestAll();

	UniqueNameTest::TestMakeNamesUnique();

	//GeneratorRewrite::Generate<CppGenerator>();

	//std::cout << "FTransform::MinAlignment: " << *reinterpret_cast<int32*>(static_cast<uint8*>(ObjectArray::FindObjectFast("Transform", EClassCastFlags::Struct)) + Off::UStruct::Size + 0x4) << std::endl;
	//Generator::GenerateSDK();
	//Generator::GenerateMappings();
	//Generator::GenerateIDAMappings();

	auto t_C = high_resolution_clock::now();
	
	auto ms_int_ = duration_cast<milliseconds>(t_C - t_1);
	duration<double, std::milli> ms_double_ = t_C - t_1;
	
	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stdout);
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