#include <Windows.h>
#include <iostream>
#include <chrono>

#include "Generators/Generator.h"
#include "Capture/MultiLevelCapture.h"

DWORD MainThread(HMODULE Module)
{
AllocConsole();
FILE* Dummy;
freopen_s(&Dummy, "CONOUT$", "w", stderr);
freopen_s(&Dummy, "CONIN$", "r", stdin);

std::cerr << "Started Generation [Dumper-7]!\n";

Settings::Config::Load();

if (Settings::Config::SleepTimeout > 0)
{
std::cerr << "Sleeping for " << Settings::Config::SleepTimeout << "ms...\n";
Sleep(Settings::Config::SleepTimeout);
}

Generator::InitEngineCore();

auto Eject = [&]()
{
fclose(stderr);
if (Dummy) fclose(Dummy);
FreeConsole();
FreeLibraryAndExitThread(Module, 0);
};

if (Settings::Config::bMultiLevelCapture)
{
std::cerr << "\n[Dumper-7] Multi-level capture mode:\n";
std::cerr << "  F4 = Capture current classes (press in each level)\n";
std::cerr << "  F5 = Generate SDK from all captured classes\n";
std::cerr << "  F6 = Eject\n\n";

while (true)
{
if (GetAsyncKeyState(VK_F4) & 1)
MultiLevelCapture::CaptureCurrentClasses();

if (GetAsyncKeyState(VK_F5) & 1)
MultiLevelCapture::GenerateSDK();

if (GetAsyncKeyState(VK_F6) & 1)
Eject();

Sleep(100);
}
}
else
{
std::cerr << "\n[Dumper-7] Controls:\n";
std::cerr << "  F5 = Generate SDK\n";
std::cerr << "  F6 = Eject\n\n";

while (true)
{
if (GetAsyncKeyState(VK_F5) & 1)
{
const auto StartTime = std::chrono::high_resolution_clock::now();

MultiLevelCapture::RunGenerators();

const auto EndTime = std::chrono::high_resolution_clock::now();
const std::chrono::duration<double, std::milli> DumpTime = EndTime - StartTime;
std::cerr << std::format("\n\nGenerating SDK took ({:.0f}ms)\n\n\n", DumpTime.count());
}

if (GetAsyncKeyState(VK_F6) & 1)
Eject();

Sleep(100);
}
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

