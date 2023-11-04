#include "GeneratorRewrite.h"

void GeneratorRewrite::InitCore()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
	Off::InSDK::InitPE(); //Must be last, relies on offsets initialized in Off::Init()
}

void GetPropertyDependency(UEProperty Prop, std::unordered_set<void*>& Store)
{
	if (Prop.IsA(EClassCastFlags::StructProperty))
	{
		Store.insert(Prop.Cast<UEStructProperty>().GetUnderlayingStruct().GetAddress());
	}
	else if (Prop.IsA(EClassCastFlags::EnumProperty))
	{
		if (auto Enum = Prop.Cast<UEEnumProperty>().GetEnum())
			Store.insert(Enum.GetAddress());
	}
	else if (Prop.IsA(EClassCastFlags::ByteProperty))
	{
		if (UEObject Enum = Prop.Cast<UEByteProperty>().GetEnum())
			Store.insert(Enum.GetAddress());
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

std::unordered_map<int32, PackageMembers> GeneratorRewrite::GatherPackages()
{
	std::unordered_map<int32, PackageMembers> OutPackages;

	for (auto Obj : ObjectArray())
	{
		const bool bIsFunction = Obj.IsA(EClassCastFlags::Function);
		const bool bIsStruct = Obj.IsA(EClassCastFlags::Struct);
		const bool bIsClass = Obj.IsA(EClassCastFlags::Class);
		const bool bIsEnum = Obj.IsA(EClassCastFlags::Enum);

		PackageMembers& Members = OutPackages[Obj.GetOutermost().GetIndex()];

		if (bIsFunction)
		{
			UEProperty RetProperty = Obj.Cast<UEFunction>().GetReturnProperty();

			if (RetProperty)
				GetPropertyDependency(RetProperty, /* std::unrodered_set<void*> DepIdx */);

			Members.Functions.push_back(Obj.GetIndex());
		}
		else if (bIsStruct)
		{
			UEStruct Struct = Obj.Cast<UEStruct>();

			for (UEProperty Property : Struct.GetProperties())
			{
				GetPropertyDependency(Property, /* std::unrodered_set<void*> DepIdx */);
			}

			bIsClass ? Members.Classes.AddDependency(Obj.GetIndex(), /* DepIdx */) : Members.Structs.AddDependency(Obj.GetIndex(), /* DepIdx */);
		}
		else if (bIsEnum)
		{
			Members.Enums.push_back(Obj.GetIndex());
		}
	}
}


void GeneratorRewrite::Init()
{
	// Get all packages and their members
	std::unordered_map<int32, PackageMembers> PackagesWithMembers /* = PropertyManager::GetPackageMembers()*/;

	// Create DependencyManager, containing all packages and packages they depend on, for SDK.hpp generation
	Packages /* = */;

	// Create StructManager with all structs and their names
	
	// 
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