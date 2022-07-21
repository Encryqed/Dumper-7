#include "Generator.h"

Generator::Generator()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();
}

void Generator::GenerateSDK()
{
	Generator SDKGen;

	auto ObjectPackages = ObjectArray::GetAllPackages();

	std::cout << "Started Generation [Dumper-7]!\n";
	std::cout << "Total Packages: " << ObjectPackages.size() << "\n\n";

	fs::path GenFolder(Settings::SDKGenerationPath);
	fs::path SDKFolder = GenFolder / "SDK";

	if (fs::exists(GenFolder))
	{
		std::cout << "Removed: " << fs::remove_all(GenFolder) << "\n";
	}
	else
	{
		std::cout << "nahui blijat\n";
	}

	fs::create_directory(GenFolder);
	fs::create_directory(SDKFolder);

	for (auto& Pair : ObjectPackages)
	{
		UEObject Object = ObjectArray::GetByIndex(Pair.first);

		Package Pack(Object);
		Pack.Process(Pair.second);

		if (!Pack.IsEmpty())
		{
			std::string PackageName = Object.GetName();

			FileWriter ClassFile(SDKFolder, PackageName, FileWriter::FileType::Class);
			FileWriter StructsFile(SDKFolder, PackageName, FileWriter::FileType::Struct);
			FileWriter FunctionFile(SDKFolder, PackageName, FileWriter::FileType::Function);
			FileWriter ParameterFile(SDKFolder, PackageName, FileWriter::FileType::Parameter);

			ClassFile.WriteClasses(Pack.AllClasses);
			StructsFile.WriteStructs(Pack.AllStructs);
			FunctionFile.WriteFunctions(Pack.AllFunctions);

			for (auto Function : Pack.AllFunctions)
			{
				ParameterFile.WriteStruct(Function.GetParamStruct());
			}
		}
		else
		{
			ObjectPackages.erase(Pair.first);
		}
	}

	SDKGen.GenerateSDKHeader(GenFolder, ObjectPackages);
	SDKGen.GenerateFixupFile(GenFolder);

	std::cout << "\n\n[=] Done [=]\n\n";
}

void Generator::GenerateSDKHeader(fs::path& SdkPath, std::unordered_map<int32_t, std::vector<int32_t>>& Packages)
{
	std::ofstream HeaderStream(SdkPath / "SDK.hpp");

	HeaderStream << "#pragma once\n\n";
	HeaderStream << "// Made with <3 by Encryqed && me\n";
	HeaderStream << std::format("// {} SDK\n\n", Settings::GameName);

	HeaderStream << "#include <string>\n";
	HeaderStream << "#include <Windows.h>\n";
	HeaderStream << "#include <iostream>\n\n";

	HeaderStream << "typedef __int8 int8;\n";
	HeaderStream << "typedef __int16 int16;\n";
	HeaderStream << "typedef __int32 int32;\n";
	HeaderStream << "typedef __int64 int64;\n\n";
	
	HeaderStream << "typedef unsigned __int8 uint8;\n";
	HeaderStream << "typedef unsigned __int16 uint16;\n";
	HeaderStream << "typedef unsigned __int32 uint32;\n";
	HeaderStream << "typedef unsigned __int64 uint64;\n\n\n";
	
	HeaderStream << "#include \"PropertyFixup.hpp\"\n\n";
	HeaderStream << "#include \"SDK/Basic.hpp\"\n";

	for (auto& Pair : Packages)
	{
		std::string PackageName = ObjectArray::GetByIndex(Pair.first).GetName();

		HeaderStream << std::format("#include \"SDK/{}_structs.hpp\"\n", PackageName);
		HeaderStream << std::format("#include \"SDK/{}_classes.hpp\"\n", PackageName);
		HeaderStream << std::format("#include \"SDK/{}_parameters.hpp\"\n", PackageName);
	}
}
void Generator::GenerateFixupFile(fs::path& SdkPath)
{
	std::ofstream FixupStream(SdkPath / "PropertyFixup.hpp");

	FixupStream << "#pragma once\n\n";

	FixupStream << "//Definitions for missing Properties\n\n";

	for (auto& Property : UEProperty::UnknownProperties)
	{
		FixupStream << std::format("class {}\n{{\n\tunsigned __int8 Pad[0x{:X}];\n}}\n\n", Property.first, Property.second);
	}
}
