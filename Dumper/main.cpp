#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "Generator.h"
#include "CppGenerator.h"

#include "HashStringTableTest.h"
#include "GeneratorRewriteTest.h"
#include "StructManagerTest.h"

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

	StructManagerTest::TestAll();

	UEClass Engine = ObjectArray::FindClassFast("Engine");
	UEClass GameEngine = ObjectArray::FindClassFast("GameEngine");

	UEClass LevelStreaming = ObjectArray::FindClassFast("LevelStreaming");

	std::cout << std::format("sizeof(UEngine) = 0x{:X};\n\n", Engine.GetStructSize());
	std::cout << std::format("alignof(UEngine) = 0x{:X};\n\n", Engine.GetMinAlignment());
	std::cout << std::format("alignof(UObject) = 0x{:X};\n\n", ObjectArray::FindClassFast("Object").GetMinAlignment());
	std::cout << std::format("alignof(FTransform) = 0x{:X};\n\n", ObjectArray::FindObjectFast<UEStruct>("Transform").GetMinAlignment());
	std::cout << std::format("alignof(FVector) = 0x{:X};\n\n", ObjectArray::FindObjectFast<UEStruct>("Vector").GetMinAlignment());
	std::cout << std::format("alignof(ULevelStreaming) = 0x{:X};\n\n", LevelStreaming.GetMinAlignment());
	std::cout << std::format("alignof(ULevelStreaming::LevelTransform) = 0x{:X};\n\n", LevelStreaming.FindMember("LevelTransform").Cast<UEStructProperty>().GetUnderlayingStruct().GetMinAlignment());

	std::cout << std::format("UEngine = {};\n\n", Engine.GetAddress());
	std::cout << std::format("UGameEngine = {};\n\n", GameEngine.GetAddress()); 

	for (UEProperty Prop : GameEngine.GetProperties())
	{
		std::cout << std::format("offsetof({}) = 0x{:X};\n", Prop.GetName(), Prop.GetOffset());
	}

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