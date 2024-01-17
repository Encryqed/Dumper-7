#include "GeneratorRewrite.h"
#include "HashStringTable.h"
#include "StructManager.h"
#include "EnumManager.h"
#include "MemberManager.h"
#include "PackageManager.h"

namespace FileNameHelper
{
	inline void MakeValidFileName(std::string& InOutName)
	{
		for (char& c : InOutName)
		{
			if (c == '<' || c == '>' || c == ':' || c == '\"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
				c = '_';
		}
	}
}

void GeneratorRewrite::InitEngineCore()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
	Off::InSDK::ProcessEvent::InitPE(); //Must be last, relies on offsets initialized in Off::Init()

	Off::InSDK::Text::InitTextOffsets(); //Must here, relies on offsets initialized in Off::InitPE()
}

void GeneratorRewrite::InitInternal()
{
	// Initialize StructManager with all packages, their names, structs, classes enums, functions and dependencies
	PackageManager::Init();

	// Initialize StructManager with all structs and their names
	StructManager::Init();

	// Initialize EnumManager with all enums and their names
	EnumManager::Init();

	// Initialized all Member-Name collisions
	MemberManager::Init();
}

bool GeneratorRewrite::SetupDumperFolder()
{
	try
	{
		std::string FolderName = (Settings::GameVersion + '-' + Settings::GameName);

		FileNameHelper::MakeValidFileName(FolderName);

		DumperFolder = fs::path(Settings::SDKGenerationPath) / FolderName;

		if (fs::exists(DumperFolder))
		{
			fs::path Old = DumperFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(DumperFolder, Old);
		}

		fs::create_directory(DumperFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return false;
	}

	return true;
}

bool GeneratorRewrite::SetupFolders(std::string& FolderName, fs::path& OutFolder)
{
	fs::path Dummy;
	std::string EmptyName = "";
	return SetupFolders(FolderName, OutFolder, EmptyName, Dummy);
}

bool GeneratorRewrite::SetupFolders(std::string& FolderName, fs::path& OutFolder, std::string& SubfolderName, fs::path& OutSubFolder)
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

		fs::create_directory(OutFolder);

		if (!SubfolderName.empty())
			fs::create_directory(OutSubFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return false;
	}

	return true;
}