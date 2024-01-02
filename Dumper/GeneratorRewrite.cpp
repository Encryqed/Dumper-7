#include "GeneratorRewrite.h"
#include "HashStringTable.h"
#include "StructManager.h"
#include "EnumManager.h"
#include "MemberManager.h"

void GeneratorRewrite::InitEngineCore()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
	Off::InSDK::InitPE(); //Must be last, relies on offsets initialized in Off::Init()
}

void GetPropertyDependency(UEProperty Prop, std::unordered_set<int32>& Store)
{
	if (Prop.IsA(EClassCastFlags::StructProperty))
	{
		Store.insert(Prop.Cast<UEStructProperty>().GetUnderlayingStruct().GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::EnumProperty))
	{
		if (auto Enum = Prop.Cast<UEEnumProperty>().GetEnum())
			Store.insert(Enum.GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::ByteProperty))
	{
		if (UEObject Enum = Prop.Cast<UEByteProperty>().GetEnum())
			Store.insert(Enum.GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::ArrayProperty))
	{
		GetPropertyDependency(Prop.Cast<UEArrayProperty>().GetInnerProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::SetProperty))
	{
		GetPropertyDependency(Prop.Cast<UESetProperty>().GetElementProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::MapProperty))
	{
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetKeyProperty(), Store);
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetValueProperty(), Store);
	}
}

std::unordered_map<int32, PackageInfo> GeneratorRewrite::GatherPackages()
{
	std::unordered_map<int32, PackageInfo> OutPackages;

	for (auto Obj : ObjectArray())
	{
		const bool bIsFunction = Obj.IsA(EClassCastFlags::Function);
		const bool bIsStruct = Obj.IsA(EClassCastFlags::Struct);
		const bool bIsClass = Obj.IsA(EClassCastFlags::Class);
		const bool bIsEnum = Obj.IsA(EClassCastFlags::Enum);

		int32 PackageIdx = Obj.GetOutermost().GetIndex();

		PackageInfo& CurrentPackageInfo = OutPackages[PackageIdx];

		std::unordered_set<int32> Dependencies;

		if (bIsFunction)
		{
			UEProperty RetProperty = Obj.Cast<UEFunction>().GetReturnProperty();

			if (RetProperty)
				GetPropertyDependency(RetProperty, Dependencies);

			// Dont move this loop, 'Dependencies' is moved from later
			for (int32 Dependency : Dependencies)
			{
				CurrentPackageInfo.PackageDependencies.insert(ObjectArray::GetByIndex(Dependency).GetOutermost().GetIndex());
			}

			CurrentPackageInfo.Functions.push_back(Obj.GetIndex());
		}
		else if (bIsStruct)
		{
			UEStruct Struct = Obj.Cast<UEStruct>();

			for (UEProperty Property : Struct.GetProperties())
			{
				GetPropertyDependency(Property, Dependencies);
			}

			// Dont move this loop, 'Dependencies' is moved from later
			for (int32 Dependency : Dependencies)
			{
				CurrentPackageInfo.PackageDependencies.insert(ObjectArray::GetByIndex(Dependency).GetOutermost().GetIndex());
			}

			DependencyManager& Manager = bIsClass ? CurrentPackageInfo.Classes : CurrentPackageInfo.Structs;
			Manager.SetDependencies(Obj.GetIndex(), std::move(Dependencies));
		}
		else if (bIsEnum)
		{
			CurrentPackageInfo.Enums.push_back(Obj.GetIndex());
		}
	}

	return OutPackages;
}


void GeneratorRewrite::InitInternal()
{
	// Create DependencyManager, containing all packages and packages they depend on, for SDK.hpp generation
	Packages = GatherPackages();

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