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
	Generator::GenerateSDK();
	auto t_2 = high_resolution_clock::now();
	
	auto ms_int_ = duration_cast<milliseconds>(t_2 - t_1);
	duration<double, std::milli> ms_double_ = t_2 - t_1;
	
	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	//Generator SDKGen;
	
	//for (auto Obj : ObjectArray())
	//{
	//	if (Obj.IsA(EClassCastFlags::UStructProperty) && !Obj.HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
	//	{
	//	}
	//}
	// 
	//Package myPack(ObjectArray::FindObjectFast("GameplayTags", EClassCastFlags::UPackage));
	//
	//std::vector<int32> PackageMembers;
	//
	//for (UEObject Obj : ObjectArray())
	//{
	//	if(Obj.IsA(EClassCastFlags::UStruct) && !Obj.IsA(EClassCastFlags::UFunction) && !Obj.HasAnyFlags(EObjectFlags::RF_ClassDefaultObject) && Obj.GetOutermost().GetName() == "GameplayTags")
	//		PackageMembers.push_back(Obj.GetIndex());
	//}
	//
	//std::unordered_map<int32, std::vector<int32>> Out;
	//ObjectArray::GetAllPackages(Out);
	//
	//std::cout << "Processing\n";
	////myPack.Process(PackageMembers);
	//myPack.Process(Out[myPack.DebugGetObject().GetIndex()]);
	//
	//fs::path GenFolder(Settings::SDKGenerationPath);
	//fs::path SDKFolder = GenFolder / "SDK";
	//
	//if (fs::exists(GenFolder))
	//	std::cout << "Removed: 0x" << fs::remove_all(GenFolder) << " Files!\n";
	//
	//fs::create_directory(GenFolder);
	//fs::create_directory(SDKFolder);
	//
	//FileWriter fwc(SDKFolder, "GameplayTags", FileWriter::FileType::Class);
	//FileWriter fws(SDKFolder, "GameplayTags", FileWriter::FileType::Struct);
	//
	//std::cout << "Writing\n";
	//
	//std::cout << "myPack.AllClasses: 0x" << std::hex << myPack.AllClasses.size() << "\n";
	//std::cout << "myPack.AllStructs: 0x" << std::hex << myPack.AllStructs.size() << "\n";
	//
	//fwc.WriteClasses(myPack.AllClasses);
	//fws.WriteStructs(myPack.AllStructs);
	//
	//fwc.Close();
	//fws.Close();

	std::cout << "done!\n";

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