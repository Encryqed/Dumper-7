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

	std::cout << "Dumper-7 started!\n";
	std::cout << "Total Packages: " << (int)ObjectPackages.size() << "\n\n";

	for (auto Pair : ObjectPackages)
	{
		UEObject Object = ObjectArray::GetByIndex(Pair.first);

		Package Pack(Object);
		Pack.Process(Pair.second);

		std::filesystem::create_directory("SDK");

		FileWriter ClassFile("SDK/" + Object.GetName(), FileWriter::FileType::Class);
		FileWriter StructsFile("SDK/" + Object.GetName(), FileWriter::FileType::Struct);
		FileWriter FunctionFile("SDK/" + Object.GetName(), FileWriter::FileType::Function);

		if (!Pack.IsEmpty())
		{
			ClassFile.WriteClasses(Pack.AllClasses);
			StructsFile.WriteStructs(Pack.AllStructs);
			// Parameter
			FunctionFile.WriteFunctions(Pack.AllFunctions);
		}

		ClassFile.Close();
	}

	std::cout << "\n\n[=] Done with Aids [=]\n\n";
}
