#pragma once

#include <unordered_set>
#include <filesystem>
#include <chrono>
#include "FileWriter.h"
#include "Settings.h"
#include "Package.h"

namespace fs = std::filesystem;

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

class Generator
{
	friend class Package;
	friend class Types::Class;
	friend class FileWriter;

private:
	struct PredefinedFunction
	{
		std::string DeclarationH;
		std::string DeclarationCPP;
		std::string Body;
	};

	struct PredefinedMember
	{
		std::string Type;
		std::string Name;
		uint32 Offset;
		uint32 Size;
	};

	typedef std::unordered_map<std::string, std::pair<std::string, std::vector<PredefinedFunction>>> FunctionsMap;
	typedef std::unordered_map<std::string, std::vector<PredefinedMember>> MemberMap;

	static FunctionsMap PredefinedFunctions; // Types.cpp
	static MemberMap PredefinedMembers; // Package.cpp

public:
	Generator();

	static void InitPredefinedMembers();
	static void InitPredefinedFunctions();

	static void GenerateSDK();

	void GenerateSDKHeader(fs::path& SdkPath, std::unordered_map<int32_t, std::vector<int32_t>>& Packages);
	void GenerateFixupFile(fs::path& SdkPath);
	void GenerateBasicFile(fs::path& SdkPath);
};
