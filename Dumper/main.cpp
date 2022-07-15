#include <Windows.h>
#include <iostream>
#include <chrono>
#include "Enums.h"
#include "ObjectArray.h"
#include "Utils.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	ObjectArray::Initialize();

	{
		auto t_1 = high_resolution_clock::now();
		for (int i = 0; i < 0x1; i++)
		{
			if (ObjectArray::FindObject("HansMeier der krasse ficker!"))
			{
				std::cout << "ddl\n\n";
			}
		}
		auto t_2 = high_resolution_clock::now();
	
		auto ms_int_ = duration_cast<milliseconds>(t_2 - t_1);
		duration<double, std::milli> ms_double_ = t_2 - t_1;
	
		std::cout << "\n\nComparing 1k times took (" << ms_int_.count() << "ms)\n\n\n";
	}

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
