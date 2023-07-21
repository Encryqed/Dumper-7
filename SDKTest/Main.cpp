

#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include "SDK.hpp"
//#include "Assertions.h"

using namespace SDK;


DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	InitGObjects();

	std::cout << UObject::GObjects << "\n";
	std::cout << UObject::GObjects->Num() << "\n\n";
	std::cout << UObject::GObjects->GetByIndex(69)->GetName() << "\n\n";
	std::cout << UEngine::StaticClass()->GetName() << "\n\n";
	std::cout << UObject::FindClassFast("Engine")->GetFullName() << "\n\n";

	UKismetSystemLibrary* Kismet = reinterpret_cast<UKismetSystemLibrary*>(UKismetSystemLibrary::StaticClass()->DefaultObject);

	std::cout << Kismet->GetGameName().ToString() << std::endl;


	UEngine* GEngine = nullptr;

	for (int i = 0; i < UObject::GObjects->Num(); i++)
	{
		UObject* Obj = UObject::GObjects->GetByIndex(i);

		if (!Obj)
			continue;

		if (Obj->IsA(UEngine::StaticClass()) && !Obj->IsDefaultObject())
		{
			std::cout << Obj->GetFullName() << "\n";
			GEngine = reinterpret_cast<UEngine*>(Obj);
		}
	}

	std::cout << GEngine->GameViewport->World->GetFullName() << std::endl;

	FGuid First = { 40, 30, 20, 20 };
	FGuid Second = { 40, 10, 20, 10 };
	FVector FirstVec = { 40.0f, 30.0f, 20.0f };
	FVector SecondVec = { 40.0f, 10.0f, 20.0f };

	if (First == Second && First != Second && FirstVec == SecondVec || FirstVec == SecondVec)
		std::cout << "what";


	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			Sleep(200);

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