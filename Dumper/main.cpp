#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include "CppGenerator.h"
#include "MappingGenerator.h"
#include "IDAMappingGenerator.h"

#include "StructManager.h"
#include "EnumManager.h"

#include "HashStringTableTest.h"
#include "GeneratorRewriteTest.h"
#include "StructManagerTest.h"
#include "CollisionManagerTest.h"
#include "MemberManagerTest.h"
#include "CppGeneratorTest.h"
#include "EnumManagerTest.h"
#include "PackageManagerTest.h"

#include "GeneratorRewrite.h"

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

	auto t_1 = std::chrono::high_resolution_clock::now();

	std::cout << "Started Generation [Dumper-7]!\n";
  
	GeneratorRewrite::InitEngineCore();
	GeneratorRewrite::InitInternal();

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


	//CppGeneratorTest::TestAll<true>();
	//HashStringTableTest::TestAll<true>();
	//StructManagerTest::TestAll<true>();
	//EnumManagerTest::TestAll<true>();
	//CollisionManagerTest::TestAll<true>();
	//MemberManagerTest::TestAll<true>();

	//PackageManagerTest::TestAll<true>();
	//PackageManagerTest::TestFindCyclidDependencies<true>();
	//PackageManagerTest::TestCyclicDependencyDetection<true>();


	GeneratorRewrite::Generate<CppGenerator>();
	//GeneratorRewrite::Generate<MappingGenerator>();
	//GeneratorRewrite::Generate<IDAMappingGenerator>();

	//or (UEObject Obj : ObjectArray())
	//
	//	if (!Obj.IsA(EClassCastFlags::Function))
	//		continue;
	//
	//	if (Obj.GetName().ends_with("__DelegateSignature"))
	//	{
	//		std::cout << "__DelegateSignature: CastFlags = (" << Obj.GetClass().StringifyCastFlags() << ")" << std::endl;
	//		
	//		std::cout << "Name: " << Obj.GetFullName() << "\n" << std::endl;
	//		//if (!Obj.IsA(EClassCastFlags::DelegateFunction))
	//	}
	//

	auto t_C = std::chrono::high_resolution_clock::now();
	
	auto ms_int_ = std::chrono::duration_cast<std::chrono::milliseconds>(t_C - t_1);
	std::chrono::duration<double, std::milli> ms_double_ = t_C - t_1;
	
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