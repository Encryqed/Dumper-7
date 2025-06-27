#include "Settings.h"
#include <Windows.h>
#include <filesystem>
#include <string>

void Settings::Config::Load()
{
	namespace fs = std::filesystem;

	// Try local Dumper-7.ini
	const std::string LocalPath = (fs::current_path() / "Dumper-7.ini").string();
	const char* ConfigPath = nullptr;

	if (fs::exists(LocalPath)) 
	{
		ConfigPath = LocalPath.c_str();
	}
	else if (fs::exists(GlobalConfigPath)) // Try global path
	{
		ConfigPath = GlobalConfigPath;
	}

	// If no config found, use defaults
	if (!ConfigPath) 
		return;

	char SDKNamespace[256] = {};
	GetPrivateProfileStringA("Settings", "SDKNamespaceName", "SDK", SDKNamespace, sizeof(SDKNamespace), ConfigPath);

	SDKNamespaceName = SDKNamespace;
	SleepTimeout = max(GetPrivateProfileIntA("Settings", "SleepTimeout", 0, ConfigPath), 0);
}
