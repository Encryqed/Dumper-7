#define WIN32_LEAN_AND_MEAN   
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <windows.h>

#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Generators/Generator.h"

// kind of doesnt need to be its own function but doesnt hurt either
std::filesystem::path get_module_path(HMODULE module)
{
    WCHAR buf[256];
    return GetModuleFileNameW(module, buf, ARRAYSIZE(buf)) ? buf : std::filesystem::path();
}

// Using atomics to allow waiting for keypress in a separate thread without race conditions
std::atomic<bool> keyPressed = false;

// Reads ints from the ini file, use the actual value for VK scancodes, e.g. 0x77 = F8
void listener(int key)
{
    while (!keyPressed)
    {
        keyPressed = GetAsyncKeyState(key) & 0x8000;
    }
    Sleep(50);
}


enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};


// Kind of had to make this a separate function for the control flow
static int Dump(HMODULE Module) {
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stderr);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    auto t_1 = std::chrono::high_resolution_clock::now();

    std::cerr << "Started Generation [Dumper-7]!\n";
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
        Settings::Internal::bUseLargeWorldCoordinates = Version.ToString().find("UE4") != std::string::npos ? false : true;
    }

    std::cerr << "GameName: " << Settings::Generator::GameName << "\n";
    std::cerr << "GameVersion: " << Settings::Generator::GameVersion << "\n\n";
    Generator::Generate<MappingGenerator>();
    Generator::Generate<CppGenerator>();
    Generator::Generate<IDAMappingGenerator>();
    Generator::Generate<DumpspaceGenerator>();

    auto t_C = std::chrono::high_resolution_clock::now();

    auto ms_int_ = std::chrono::duration_cast<std::chrono::milliseconds>(t_C - t_1);
    std::chrono::duration<double, std::milli> ms_double_ = t_C - t_1;

    std::cerr << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";
    
    // wait a few seconds and then unload automatically
    // If someone wants to prevent this they can just click on the console output to pause execution
    // actually this change doesn't matter, but I felt like it was odd to have an exit key and not have it configurable
    // maybe just reuse the dumpkey if its given or just revert this portion idc
    Sleep(5000);
    fclose(stderr);
    if (Dummy) fclose(Dummy);
    FreeConsole();

    FreeLibraryAndExitThread(Module, 0);
    return 0;
}



DWORD MainThread(HMODULE Module) {
    // Grab the path to the dll in case you want to store your settings file adjacent to it and have it out of the game directory
    Settings::Config::ModulePath = get_module_path(Module).string();
    Settings::Config::Load();
    std::cerr << Settings::Generator::SDKGenerationPath;
    static auto t_0 = std::chrono::high_resolution_clock::now();
    // Dump immediately if no timeout or dump key is set
    if (Settings::Config::SleepTimeout + Settings::Config::DumpKey == 0)
        return Dump(Module);
    // If a timeout is set without a dumpkey we will just synchronously wait and then dump
    if (Settings::Config::DumpKey == 0) {
        Sleep(Settings::Config::SleepTimeout);
        return Dump(Module);
    }
    // When a dump key is set we need to suspend execution and wait asynchronously to avoid unloading or dumping early
    else {
        std::thread listenerThread(&listener, Settings::Config::DumpKey);
        listenerThread.detach();
        auto t_x = std::chrono::high_resolution_clock::now();
        while (!keyPressed)
        {
        // And finally in the case that both a dump key and timeout are set we will continue checking back on this thread
            if (Settings::Config::SleepTimeout > 0) {
                t_x = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_x - t_0).count();
                if (diff >= Settings::Config::SleepTimeout) {
                    std::cerr << "Timeout reached, dumping SDK...\n";
                    return Dump(Module);
                }
            }
            Sleep(100);
        }
        return Dump(Module);
    }

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
