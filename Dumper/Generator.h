#pragma once

#include <unordered_set>
#include <filesystem>
#include <chrono>
#include <future>
#include <set>
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
	friend class Types::Struct;
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
		int32 Offset;
		int32 Size;

		inline bool operator<(const PredefinedMember& Other) const { return  Offset != Other.Offset ? Offset < Other.Offset : Other.Size != 0; }
	};

	typedef std::unordered_map<std::string, std::pair<std::string, std::vector<PredefinedFunction>>> FunctionsMap;
	typedef std::unordered_map<std::string, std::set<PredefinedMember>> MemberMap;

	static FunctionsMap PredefinedFunctions; // Types.cpp
	static MemberMap PredefinedMembers; // Package.cpp

	static std::mutex PackageMutex;
	static std::vector<std::future<void>> Futures;

public:
	static void Init();

private:
	static void InitPredefinedMembers();
	static void InitPredefinedFunctions();

private:
	static void HandlePackageGeneration(const fs::path* const SDKFolder, int32 PackageIndex, std::vector<int32>* MemberIndices);

public:
	static void GenerateMappings();
	static void GenerateSDK();

private:
	static void GenerateSDKHeader(const fs::path& SdkPath, int32 BiggestPackageIdx);
	static void GenerateFixupFile(const fs::path& SdkPath);
	static void GenerateBasicFile(const fs::path& SdkPath);
};
