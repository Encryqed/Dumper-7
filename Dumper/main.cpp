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

	/*
	UEClass BPGenClass = ObjectArray::FindClassFast("BlueprintGeneratedClass");
	for (UEObject Obj : ObjectArray())
	{
		if (Obj.IsA(EClassCastFlags::Class) && Obj.IsA(BPGenClass))
			std::cout << "BPClass: " << Obj.GetFullName() << "\n\n";
	
		if (!Obj.IsA(EClassCastFlags::Function) || Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject))
			continue;
	
		UEFunction Func = Obj.Cast<UEFunction>();
	
		//if (Func.GetOutermost().GetName() != "Engine")
		//	continue;
	
		if (Func.GetScriptBytes().IsEmpty())
			continue;
	
		std::cout << "Flags: (" << Func.StringifyObjFlags() << ")\nObj: " << Func.GetFullName() << "\nScriptBytes: " << Func.GetScriptBytes().Num() 
			<< "\nOuter: " << Func.GetOuter().GetFullName() << "\n\n";


		//std::string DisassembledScript = Func.DumpScriptBytecode();
		//std::cout << "\n" << std::endl;
	}

	//UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("SetLoadingScreenDescription", "WBP_HDLoadingScreenBase_C");
	//UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("Construct", "WBP_DlgBox_ServerUGCDownloadStatus_C");
	//UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("ExecuteUbergraph_WBP_HDMenuButton_ModalDialog", "WBP_HDMenuButton_ModalDialog_C");
	//UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("UpdateDesignerView", "WBP_Toggle_C");
	//UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("SetToggle", "WBP_Toggle_C");
	UEFunction Func = ObjectArray::FindObjectFastInOuter<UEFunction>("GetDefaultLeftHandIKTransformByItemType", "ABP_HDPlayerCharacter_TP_C");

	std::cout << "Func: " << Func.GetName() << std::endl;

	std::string DisassembledScript = Func.DumpScriptBytecode();

	//std::cout << "SetLoadingScreenDescription Script:\n" << DisassembledScript << std::endl;
	*/
	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();


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