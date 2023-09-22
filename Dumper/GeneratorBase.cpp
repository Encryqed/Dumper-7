#include "GeneratorBase.h"

void GeneratorBase::Init()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
	Off::InSDK::InitPE(); //Must be last, relies on offsets initialized in Off::Init()
}

void GeneratorBase::SetupFolders(const std::string& FolderName, fs::path& OutFolder)
{
	fs::path Dummy;
	SetupFolders(FolderName, OutFolder, "", Dummy);
}

void GeneratorBase::SetupFolders(const std::string& FolderName, fs::path& OutFolder, const std::string& SubfolderName, fs::path& OutSubFolder)
{
	try
	{
		fs::path BaseGenerationFolder = Settings::SDKGenerationPath;
		OutFolder = BaseGenerationFolder / FolderName;
		OutSubFolder = OutFolder / SubfolderName;

		if (!fs::exists(BaseGenerationFolder))
			fs::create_directories(BaseGenerationFolder);
		
		if (fs::exists(OutFolder))
		{
			fs::path Old = OutFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(OutFolder, Old);
		}

		fs::create_directory(OutFolder);

		if (!SubfolderName.empty())
			fs::create_directory(SubfolderName);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return;
	}
}