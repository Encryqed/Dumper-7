#include "Settings.h"
#include <Windows.h>

void Settings::Config::Load(const char* path)
{
    char sdkNamespace[256] = {};
    GetPrivateProfileStringA("Settings", "SDKNamespaceName", "SDK", sdkNamespace, sizeof(sdkNamespace), path);
    SDKNamespaceName = sdkNamespace;

    SleepTimeout = GetPrivateProfileIntA("Settings", "SleepTimeout", 1000, path);
}