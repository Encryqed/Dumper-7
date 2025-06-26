#include "Settings.h"
#include <Windows.h>
#include <filesystem>
#include <string>

void Settings::Config::Load()
{
    namespace fs = std::filesystem;

    // Try local Dumper-7.ini
    std::string localPath = (fs::current_path() / "Dumper-7.ini").string();
    const char* configPath = nullptr;

    if (fs::exists(localPath)) {
        configPath = localPath.c_str();
    }
    // Try global path
    else if (fs::exists("C:/Dumper-7/Dumper-7.ini")) {
        configPath = "C:/Dumper-7/Dumper-7.ini";
    }

    // If no config found, use defaults
    if (!configPath) {
        return;
    }

    char sdkNamespace[256] = {};
    GetPrivateProfileStringA("Settings", "SDKNamespaceName", "SDK", sdkNamespace, sizeof(sdkNamespace), configPath);
    SDKNamespaceName = sdkNamespace;

    SleepTimeout = GetPrivateProfileIntA("Settings", "SleepTimeout", 0, configPath);
}
