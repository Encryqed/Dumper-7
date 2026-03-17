#include "Capture/MultiLevelCapture.h"

#include <iostream>
#include <chrono>
#include <unordered_set>

#include "Generators/Generator.h"
#include "Generators/CppGenerator.h"
#include "Generators/MappingGenerator.h"
#include "Generators/IDAMappingGenerator.h"
#include "Generators/DumpspaceGenerator.h"

#include "Unreal/ObjectArray.h"
#include "Settings.h"

static std::unordered_set<int32> RootedClasses;

void MultiLevelCapture::CaptureCurrentClasses()
{
	int32 NewCount = 0;
	for (int32 i = 0; i < ObjectArray::Num(); i++)
	{
		UEObject Obj = ObjectArray::GetByIndex(i);
		if (!Obj || !Obj.IsA(EClassCastFlags::Class)) continue;

		const int32 Index = Obj.GetIndex();
		if (!RootedClasses.insert(Index).second) continue;

		ObjectArray::AddObjectToRoot(Index);
		NewCount++;
	}

	std::cerr << std::format("Captured {} new classes ({} total rooted)\n", NewCount, RootedClasses.size());
	std::cerr << "Load the next level and press F4 again, or press F5 to generate the SDK.\n";
}

void MultiLevelCapture::RunGenerators()
{
	if (Settings::Generator::GameName.empty() && Settings::Generator::GameVersion.empty())
	{
		FString Name;
		FString Version;
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEFunction GetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
		UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");

		Kismet.ProcessEvent(GetGameName, &Name);
		Kismet.ProcessEvent(GetEngineVersion, &Version);

		Settings::Generator::GameName = Name.ToString();
		Settings::Generator::GameVersion = Version.ToString();
	}

	std::cerr << std::format("GameName: {}\n", Settings::Generator::GameName);
	std::cerr << std::format("GameVersion: {}\n\n", Settings::Generator::GameVersion);
	std::cerr << std::format("FolderName: {}\n\n", Settings::Generator::GameVersion + '-' + Settings::Generator::GameName);

	Generator::InitInternal();

	Generator::Generate<CppGenerator>();
	Generator::Generate<MappingGenerator>();
	Generator::Generate<IDAMappingGenerator>();
	Generator::Generate<DumpspaceGenerator>();
}

void MultiLevelCapture::GenerateSDK()
{
	if (RootedClasses.empty())
	{
		std::cerr << "No classes captured yet - press F4 first.\n";
		return;
	}

	std::cerr << std::format("Generating SDK from {} rooted classes...\n", RootedClasses.size());
	const auto StartTime = std::chrono::high_resolution_clock::now();

	RunGenerators();

	const auto EndTime = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double, std::milli> DumpTime = EndTime - StartTime;
	std::cerr << std::format("\n\nGenerating SDK took ({:.0f}ms)\n\n\n", DumpTime.count());
}
