#include <iostream>
#include <filesystem>
#include <Windows.h>

int main()
{
	auto DLLPath = std::filesystem::current_path().string() + "\\Dumper-7.dll";

	// DLL is in the same path as the injector, atleast it should be.
	if (!std::filesystem::exists(DLLPath))
	{
		std::cout << "Target DLL file not found, exiting!";
		return 1;
	}

	// Should work for all UE games
	auto GameWindow = FindWindowA("UnrealWindow", 0);
	if (!GameWindow)
	{
		std::cout << "Game window not found, exiting!";
		return 1;
	}

	DWORD GamePID = 0;
	GetWindowThreadProcessId(GameWindow, &GamePID);
	if (GamePID == 0)
	{
		std::cout << "Game process ID not found, exiting!";
		return 1;
	}

	auto GameHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, GamePID);
	if (!GameHandle)
	{
		std::cout << "Couldn't open the game process, exiting!";
		return 1;
	}

	auto DLLPathAddr = VirtualAllocEx(GameHandle, 0, lstrlenA(DLLPath.c_str()) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!DLLPathAddr)
	{
		std::cout << "Couldn't allocate memory in the game process, exiting!";
		return 1;
	}

	if (WriteProcessMemory(GameHandle, DLLPathAddr, DLLPath.c_str(), lstrlenA(DLLPath.c_str()) + 1, 0) == 0)
	{
		std::cout << "Couldn't write the DLL path in the game process, exiting!";
		return 1;
	}

	auto K32Module = GetModuleHandleA("kernel32.dll");
	if (!K32Module || K32Module == INVALID_HANDLE_VALUE)
	{
		std::cout << "Couldn't get a handle to kernel32.dll, exiting!";
		return 1;
	}

	auto LoadLibraryAddr = GetProcAddress(K32Module, "LoadLibraryA");
	if (!LoadLibraryAddr)
	{
		std::cout << "Couldn't get LoadLibraryA, exiting!";
		return 1;
	}

	auto RemoteThread = CreateRemoteThread(GameHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryAddr, DLLPathAddr, 0, nullptr);
	if (!RemoteThread || RemoteThread == INVALID_HANDLE_VALUE)
	{
		std::cout << "Couldn't create a remote thread in the game process, exiting!";
		return 1;
	}

	std::cout << "DLL injected successfully!\n";
	return 0;
}