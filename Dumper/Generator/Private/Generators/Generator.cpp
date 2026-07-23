
#include "Generators/Generator.h"
#include "Managers/StructManager.h"
#include "Managers/EnumManager.h"
#include "Managers/MemberManager.h"
#include "Managers/PackageManager.h"

#include "HashStringTable.h"
#include "Utils.h"

#include "Platform.h"
#include "Json/json.hpp"

#include <fstream>
#include <regex>

#pragma comment(lib, "version.lib")

inline void InitSettings()
{
	Settings::InitWeakObjectPtrSettings();
	Settings::InitLargeWorldCoordinateSettings();

	Settings::InitObjectPtrPropertySettings();
	Settings::InitArrayDimSizeSettings();
}


void Metadata::DumpEditorOnly(const fs::path& DumperFolder)
{
	if (Off::FField::EditorOnlyMetadata == -1)
		return;

	nlohmann::json MetadataJson;
	MetadataJson["EngineVersion"] = Settings::Generator::EngineVersion;
	MetadataJson["GameName"] = Settings::Generator::GameName;

	for (UEObject Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct))
			continue;

		UEStruct Struct = Obj.Cast<UEStruct>();

		auto ChildProperties = Struct.GetProperties();

		if (ChildProperties.empty())
			continue;

		auto& StructMembers = MetadataJson[Struct.GetCppName()];

		for (UEProperty Prop : Struct.GetProperties())
		{
			auto& Entries = StructMembers[Prop.GetValidName()];

			for (const auto& [Key, Value] : Prop.Cast<UEFField>().GetMetaData())
			{
				if (Key.empty() && Value.empty())
					continue;

				Entries[Key] = Value;
			}
		}
	}

	std::ofstream MetadataFile(DumperFolder / "Metadata.json");
	MetadataFile << MetadataJson.dump(4);
}


void Metadata::FetchEngineFallback()
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

void Metadata::FetchEngine(UEClass Kismet)
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


void Metadata::FetchGameFallback()
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

void Metadata::FetchGame(UEClass Kismet)
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


void Metadata::Fetch()
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


void Generator::InitEngineCore()
{
	/* manual override */
	//ObjectArray::Init(/*GObjects*/, /*Layout = Default*/); // FFixedUObjectArray (UEVersion < UE4.21)
	//ObjectArray::Init(/*GObjects*/, /*ChunkSize*/, /*Layout = Default*/); // FChunkedFixedUObjectArray (UEVersion >= UE4.21)

	//FName::Init(/*bForceGNames = false*/);
	//FName::Init(/*AppendString, FName::EOffsetOverrideType::AppendString*/);
	//FName::Init(/*ToString, FName::EOffsetOverrideType::ToString*/);
	//FName::Init(/*GNames, FName::EOffsetOverrideType::GNames, true/false*/);
 
	//Off::InSDK::ProcessEvent::InitPE(/*PEIndex*/);

	/* Back4Blood (requires manual GNames override) */
	//InitObjectArrayDecryption([](void* ObjPtr) -> uint8* { return reinterpret_cast<uint8*>(uint64(ObjPtr) ^ 0x8375); });

	/* Multiversus [Unsupported, weird GObjects-struct] */
	//InitObjectArrayDecryption([](void* ObjPtr) -> uint8* { return reinterpret_cast<uint8*>(uint64(ObjPtr) ^ 0x1B5DEAFD6B4068C); });

	ObjectArray::Init();

	CALL_PLATFORM_SPECIFIC_FUNCTION(FName::Init);

	Off::Init();
	PropertySizes::Init();

	CALL_PLATFORM_SPECIFIC_FUNCTION(Off::InSDK::ProcessEvent::InitPE); // Must be at this position, relies on offsets initialized in Off::Init()

	Off::InSDK::World::InitGWorld(); // Must be at this position, relies on offsets initialized in Off::Init()

	Off::InSDK::Text::InitTextOffsets(); // Must be at this position, relies on offsets initialized in Off::InitPE()

	InitSettings();
}

void Generator::InitInternal()
{
	// Initialize PackageManager with all packages, their names, structs, classes enums, functions and dependencies
	PackageManager::Init();

	// Initialize StructManager with all structs and their names
	StructManager::Init();
	
	// Initialize EnumManager with all enums and their names
	EnumManager::Init();
	
	// Initialized all Member-Name collisions
	MemberManager::Init();

	// Post-Initialize PackageManager after StructManager has been initialized. 'PostInit()' handles Cyclic-Dependencies detection
	PackageManager::PostInit();
}

bool Generator::SetupDumperFolder()
{
	try
	{
		std::string FolderName = Settings::Generator::GetProjectName();

		FileNameHelper::MakeValidFileName(FolderName);

		DumperFolder = fs::path(Settings::Generator::SDKGenerationPath) / FolderName;

		if (fs::exists(DumperFolder))
		{
			fs::path Old = DumperFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(DumperFolder, Old);
		}

		fs::create_directories(DumperFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cerr << "Could not create required folders! Info: \n";
		std::cerr << fe.what() << std::endl;
		return false;
	}

	return true;
}

bool Generator::SetupFolders(std::string& FolderName, fs::path& OutFolder)
{
	fs::path Dummy;
	std::string EmptyName = "";
	return SetupFolders(FolderName, OutFolder, EmptyName, Dummy);
}

bool Generator::SetupFolders(std::string& FolderName, fs::path& OutFolder, std::string& SubfolderName, fs::path& OutSubFolder)
{
	FileNameHelper::MakeValidFileName(FolderName);
	FileNameHelper::MakeValidFileName(SubfolderName);

	try
	{
		OutFolder = DumperFolder / FolderName;
		OutSubFolder = OutFolder / SubfolderName;
				
		if (fs::exists(OutFolder))
		{
			fs::path Old = OutFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(OutFolder, Old);
		}

		fs::create_directories(OutFolder);

		if (!SubfolderName.empty())
			fs::create_directories(OutSubFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cerr << "Could not create required folders! Info: \n";
		std::cerr << fe.what() << std::endl;
		return false;
	}

	return true;
}