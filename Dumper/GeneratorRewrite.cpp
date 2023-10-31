#include "GeneratorRewrite.h"

void GeneratorRewrite::Init()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
	Off::InSDK::InitPE(); //Must be last, relies on offsets initialized in Off::Init()
}

bool GeneratorRewrite::SetupDumperFolder()
{
	try
	{
		DumperFolder = fs::path(Settings::SDKGenerationPath) / (Settings::GameVersion + '-' + Settings::GameName);

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

bool GeneratorRewrite::SetupFolders(const std::string& FolderName, fs::path& OutFolder)
{
	fs::path Dummy;
	return SetupFolders(FolderName, OutFolder, "", Dummy);
}

bool GeneratorRewrite::SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder)
{
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