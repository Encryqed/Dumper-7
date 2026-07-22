#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <regex>

#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Generators/Generator.h"

#pragma comment(lib, "version.lib")

enum class EFortToastType : uint8
{
    Default                                  = 0,
    Subdued                                  = 1,
    Impactful                                = 2,
    Lock                                     = 3,
    EFortToastType_MAX                       = 4,
};

class Metadata
{
private:
	static void FetchEngineFallback()
	{
		bool WasVersionFound = false;
		std::string ProductVersion = "";

		/*
		* Invoked Lambda (IIFE) to allow early return without skipping the string generation below.
		* See: https://en.cppreference.com/cpp/language/lambda
		*/
		[&]()
		{
			char Buffer[MAX_PATH];
			if (GetModuleFileNameA(NULL, Buffer, MAX_PATH) == 0)
				return;

			DWORD DummyHandle = 0;
			DWORD InfoSize = GetFileVersionInfoSizeA(Buffer, &DummyHandle);
			if (InfoSize == 0)
				return;

			std::vector<BYTE> VersionData(InfoSize);
			if (GetFileVersionInfoA(Buffer, 0, InfoSize, VersionData.data()) == FALSE)
				return;

			/* Extract exact version numbers from the binary VS_FIXEDFILEINFO block */
			VS_FIXEDFILEINFO* FileInfo = nullptr;
			UINT FileInfoSize = 0;
			if (VerQueryValueA(VersionData.data(), "\\", (LPVOID*)&FileInfo, &FileInfoSize) && FileInfoSize > 0)
			{
				Settings::Generator::EngineMajor = HIWORD(FileInfo->dwFileVersionMS);
				Settings::Generator::EngineMinor = LOWORD(FileInfo->dwFileVersionMS);
				Settings::Generator::EnginePatch = HIWORD(FileInfo->dwFileVersionLS);
				Settings::Generator::EngineBuild = LOWORD(FileInfo->dwFileVersionLS);
				WasVersionFound = true;
			}


			/* Extract the technical build string from StringFileInfo (ProductVersion) */
			struct LANGANDCODEPAGE { WORD wLanguage; WORD wCodePage; } *lpTranslate;
			UINT cbTranslate;

			if (VerQueryValueA(VersionData.data(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate) == FALSE)
				return;

			if ((cbTranslate / sizeof(LANGANDCODEPAGE)) == 0)
				return;

			/* Construct the query path for the ProductVersion string */
			char SubBlock[MAX_PATH];
			sprintf_s(SubBlock, "\\StringFileInfo\\%04x%04x\\ProductVersion", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

			char* ProductVersionString = nullptr;
			UINT Length = 0;
			if (VerQueryValueA(VersionData.data(), SubBlock, (LPVOID*)&ProductVersionString, &Length) && Length > 0)
			{
				ProductVersion = std::string(ProductVersionString);
			}
		}();

		std::string EngineVersionString = WasVersionFound
			? std::to_string(Settings::Generator::EngineMajor) + "." + std::to_string(Settings::Generator::EngineMinor) + "." + std::to_string(Settings::Generator::EnginePatch) + "-" + std::to_string(Settings::Generator::EngineBuild)
			: "0.0.0-0";

		if (ProductVersion.empty() == false)
			EngineVersionString += ProductVersion;

		Settings::Generator::EngineVersion = EngineVersionString;
	}

	static void FetchEngine(UEClass Kismet)
	{
		if (Kismet == nullptr)
		{
			FetchEngineFallback();
			return;
		}

		UEFunction FnGetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");
		if (FnGetEngineVersion == nullptr)
		{
			FetchEngineFallback();
			return;
		}
			
		FString EngineVersion;
		Kismet.ProcessEvent(FnGetEngineVersion, &EngineVersion);

		std::string EngineVersionString = EngineVersion.ToString();
		if (EngineVersionString.empty())
		{
			FetchEngineFallback();
			return;
		}
		
		Settings::Generator::EngineVersion = EngineVersionString;

		static const std::regex EngineVersionRegex(R"((\d+)\.(\d+)\.(\d+)\-(\d+))");
		std::smatch Match;
		if (std::regex_search(EngineVersionString, Match, EngineVersionRegex))
		{
			Settings::Generator::EngineMajor = std::stoi(Match[1].str());
			Settings::Generator::EngineMinor = std::stoi(Match[2].str());
			Settings::Generator::EnginePatch = std::stoi(Match[3].str());
			Settings::Generator::EngineBuild = std::stoi(Match[4].str());
		}
	}


	static void FetchGameFallback()
	{
		std::string ExecutableName = "";

		char Buffer[MAX_PATH];
		if (GetModuleFileNameA(NULL, Buffer, MAX_PATH) > 0)
		{
			std::string ExecutablePath(Buffer);

			size_t SlashPos = ExecutablePath.find_last_of("/\\");
			size_t DotPos = ExecutablePath.find_last_of('.');

			size_t SubStrStart = (SlashPos != std::string::npos) ? SlashPos + 1 : 0;
			size_t SubStrEnd = (DotPos != std::string::npos && DotPos > SubStrStart) ? DotPos : ExecutablePath.length();

			ExecutableName = ExecutablePath.substr(SubStrStart, SubStrEnd - SubStrStart);
		}

		Settings::Generator::GameName = ExecutableName.empty() == false ? ExecutableName : "Undefined";
	}

	static void FetchGame(UEClass Kismet)
	{
		if (Kismet == nullptr)
		{
			FetchGameFallback();
			return;
		}

		UEFunction FnGetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
		if (FnGetGameName == nullptr)
		{
			FetchGameFallback();
			return;
		}
			
		FString GameName;
		Kismet.ProcessEvent(FnGetGameName, &GameName);

		std::string GameNameString = GameName.ToString();
		if (GameNameString.empty())
		{
			FetchGameFallback();
			return;
		}

		Settings::Generator::GameName = GameNameString;
	}

public:
	static void Fetch()
	{
		/* Following code is only possible from within MainThread() */
		bool NeedsEngineMetadata = Settings::Generator::EngineVersion.empty();
		bool NeedsGameMetadata = Settings::Generator::GameName.empty();

		if (NeedsEngineMetadata == false && NeedsGameMetadata == false)
			return;

		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");

		if (NeedsEngineMetadata)
			FetchEngine(Kismet);

		if (NeedsGameMetadata)
			FetchGame(Kismet);
	}
};

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONIN$", "r", stdin);
	freopen_s(&Dummy, "CONOUT$", "w", stderr);
	std::cerr.clear(); // clear internal error flags on cerr after redirect
	std::cerr << std::boolalpha << std::hex; // Switch console output to HEX, and display bools as "true"/"false"

	std::cerr << "Initializing [Dumper-7]\n";

	Settings::Config::Load();
	Settings::Config::DelayDumperStart();

	std::cerr << "Started Generation [Dumper-7]!\n";
	auto DumpStartTime = std::chrono::high_resolution_clock::now();

	Generator::InitEngineCore();
	Generator::InitInternal();

	Metadata::Fetch();

	std::cerr << std::dec; // Switch console output to decimal for metadata

	std::cerr << "EngineVersion: " << Settings::Generator::EngineVersion << "\n";
	std::cerr << "EngineMajor: " << Settings::Generator::EngineMajor << "\n";
	std::cerr << "EngineMinor: " << Settings::Generator::EngineMinor << "\n";
	std::cerr << "EnginePatch: " << Settings::Generator::EnginePatch << "\n";
	std::cerr << "EngineBuild: " << Settings::Generator::EngineBuild << "\n\n";

	std::cerr << "GameName: " << Settings::Generator::GameName << "\n\n";

	std::cerr << "FolderName: " << Settings::Generator::GetProjectName() << "\n\n";

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();

	auto DumpFinishTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> DumpTime = DumpFinishTime - DumpStartTime;

	std::cerr << "\n\nGenerating SDK took (" << DumpTime.count() << "ms)\n\n\n";

	if (Settings::Debug::bExecuteSDKTestScript)
	{
		/* Executes a python script to test if the SDK compiles correctly. */
		CppGenerator::ExecuteSDKCompilationTestScript();
	}

	std::cerr << "\n\nPress F6 to unload\n\n\n";

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stderr);
			if (Dummy) 
			{
				fclose(Dummy);
			}
			FreeConsole();

			FreeLibraryAndExitThread(Module, 0);
		}

		Sleep(100);
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
