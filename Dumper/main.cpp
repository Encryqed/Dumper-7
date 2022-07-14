#include <Windows.h>
#include <iostream>
#include "Enums.h"
#include "ObjectArray.h"

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);


	ObjectArray::Initialize();

	//add core offsets before testing, retard
	std::cout << ObjectArray::FindObject<UEObject>(ObjectArray::GetByIndex(ObjectArray::Num() - 1).GetFullName()).GetFullName() << "\n\n";

	std::cout << ObjectArray::GetByIndex(80000).GetAddress() << "\n";

	std::cout << "Objects[1500]: "   << std::dec << ObjectArray::GetByIndex(1500).GetIndex() << "\n";
	std::cout << "Objects[15000]: "  << std::dec << ObjectArray::GetByIndex(15000).GetIndex() << "\n";
	std::cout << "Objects[150000]: " << std::dec << ObjectArray::GetByIndex(150000).GetIndex() << "\n";
	std::cout << "Objects[4587]: "   << std::dec << ObjectArray::GetByIndex(4587).GetIndex() << "\n";
	std::cout << "Objects[300000]: " << std::dec << ObjectArray::GetByIndex(300000).GetIndex() << "\n\n";

	std::cout << "Objects[300000]: " << std::dec << ObjectArray::GetByIndex(300000).GetName() << "\n\n";

	int32 LastIndex = -1;
	for (auto Object : ObjectArray())
	{
		if (Object.GetFullName().length() == LastIndex)
		{
			std::cout << "Non continuos! (" << Object.GetIndex() << ")\n";
		}
	}
	std::cout << "Passed check!\n";

	std::cout << ObjectArray::GetByIndex(300000).GetAddress() << "\n";


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
