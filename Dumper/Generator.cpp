#include "Generator.h"

Generator::Generator()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();

	
}

void Generator::Start()
{
	auto ObjectPackages = ObjectArray::GetAllPackages();

	std::cout << "Started Generation [Dumper-7]!\n";
	std::cout << "Total Packages: " << ObjectPackages.size() << "\n\n";

	for (auto Pair : ObjectPackages)
	{
		UEObject Object = ObjectArray::GetByIndex(Pair.first);

		Package Pack(Object);
		Pack.Process(Pair.second);


		std::string PackageName = Object.GetName();
		std::filesystem::path GenFolder(Settings::SDKGenerationPath);
		std::filesystem::path SDKFolder = GenFolder / "SDK";

		std::filesystem::create_directory(GenFolder);
		std::filesystem::create_directory(SDKFolder);

		if (!Pack.IsEmpty())
		{
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
	}

	std::cout << "\n\n[=] Done [=]\n\n";
}
