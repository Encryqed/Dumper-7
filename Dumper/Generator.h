#pragma once

#include <filesystem>
#include "FileWriter.h"
#include "Settings.h"
#include "Package.h"

namespace fs = std::filesystem;

class Generator
{
private:
	friend class Package;

	struct PredefinedFunction
	{
		std::string DeclarationH;
		std::string DeclarationCPP;
		std::string Body;
	};

public:
	Generator();

	static void GenerateSDK();

	void GenerateSDKHeader(fs::path& SdkPath, std::unordered_map<int32_t, std::vector<int32_t>>& Packages);
	void GenerateFixupFile(fs::path& SdkPath);
};