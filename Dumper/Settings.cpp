#include "Settings.h"
#include <Windows.h>
#include <filesystem>
#include <string>

void Settings::Config::Load()
{
    namespace fs = std::filesystem;

    // Try local Dumper-7.ini
    std::string LocalPath = (fs::current_path() / "Dumper-7.ini").string();
    const char* ConfigPath = nullptr;

    if (fs::exists(LocalPath)) 
    {
        ConfigPath = LocalPath.c_str();
    }
    // Try global path
    else if (fs::exists("C:/Dumper-7/Dumper-7.ini")) 
    {
        ConfigPath = "C:/Dumper-7/Dumper-7.ini";
    }

    // If no config found, use defaults
    if (!ConfigPath) 
    {
        return;
    }

    char SDKNamespace[256] = {};
    GetPrivateProfileStringA("Settings", "SDKNamespaceName", "SDK", SDKNamespace, sizeof(SDKNamespace), ConfigPath);
    SDKNamespaceName = SDKNamespace;

    SleepTimeout = GetPrivateProfileIntA("Settings", "SleepTimeout", 0, ConfigPath);
}
