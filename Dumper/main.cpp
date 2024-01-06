#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "Generator.h"
#include "CppGenerator.h"

#include "StructManager.h"
#include "EnumManager.h"

#include "HashStringTableTest.h"
#include "GeneratorRewriteTest.h"
#include "StructManagerTest.h"
#include "CollisionManagerTest.h"
#include "MemberManagerTest.h"
#include "CppGeneratorTest.h"
#include "EnumManagerTest.h"
#include "PackageManagerTest.h"

#include "GeneratorRewrite.h"

//#include "StringTableTest.h"

enum class EFortToastType : uint8
{
        Default                        = 0,
        Subdued                        = 1,
        Impactful                      = 2,
        EFortToastType_MAX             = 3,
};

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	auto t_1 = high_resolution_clock::now();

	std::cout << "Started Generation [Dumper-7]!\n";
  
	Generator::Init();
	//GeneratorRewrite::InitEngineCore();
	GeneratorRewrite::InitInternal();

	if (Settings::GameName.empty() && Settings::GameVersion.empty())
	{
		// Only Possible in Main()
		FString Name;
		FString Version;
		UEClass Kismet = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEFunction GetGameName = Kismet.GetFunction("KismetSystemLibrary", "GetGameName");
		UEFunction GetEngineVersion = Kismet.GetFunction("KismetSystemLibrary", "GetEngineVersion");

		Kismet.ProcessEvent(GetGameName, &Name);
		Kismet.ProcessEvent(GetEngineVersion, &Version);

		Settings::GameName = Name.ToString();
		Settings::GameVersion = Version.ToString();
	}

	std::cout << "GameName: " << Settings::GameName << "\n";
	std::cout << "GameVersion: " << Settings::GameVersion << "\n\n";


	// FName is the same for both UOnlineEngineInterfaceImpls
	std::cout << std::endl;
	for (UEObject Obj : ObjectArray())
	{
		if (Obj.IsA(EClassCastFlags::Class) && Obj.GetCppName() == "UOnlineEngineInterfaceImpl")
			std::cout << "UOnlineEngineInterfaceImpl: " << Obj.GetAddress() << std::endl;
	}
	std::cout << std::endl;



	std::unordered_map<int32 /* PackageIdx */, std::string> PackagesAndForwardDeclarations;

	StructManager::Init();
	EnumManager::Init();

	const StructManager::OverrideMaptType& StructInfoMap = StructManager::GetStructInfos();
	const EnumManager::OverrideMaptType& EnumInfoMap = EnumManager::GetEnumInfos();

	for (const auto& [Index, Info] : StructInfoMap)
	{
		if (StructManager::IsStructNameUnique(Info.Name))
			continue;

		UEStruct Struct = ObjectArray::GetByIndex<UEStruct>(Index);

		PackagesAndForwardDeclarations[Struct.GetOutermost().GetIndex()] += std::format("\t{} {};\n", Struct.IsA(EClassCastFlags::Class) ? "class" : "struct", Struct.GetCppName());
	}

	for (const auto& [Index, Info] : EnumInfoMap)
	{
		if (EnumManager::IsEnumNameUnique(Info))
			continue;

		UEEnum Enum = ObjectArray::GetByIndex<UEEnum>(Index);

		PackagesAndForwardDeclarations[Enum.GetOutermost().GetIndex()] += std::format("\t{} {};\n", "eunum class", Enum.GetEnumPrefixedName());
	}

	for (const auto& [PackageIndex, ForwardDeclarations] : PackagesAndForwardDeclarations)
	{
		std::cout << std::format(R"(
namespace {} {{
{}
}}
)", ObjectArray::GetByIndex(PackageIndex).GetValidName(), ForwardDeclarations.substr(0, ForwardDeclarations.size() - 1));
	}


	//CppGeneratorTest::TestAll();
	//HashStringTableTest::TestAll();
	//GeneratorRewriteTest::TestAll();
	//StructManagerTest::TestAll();
	//EnumManagerTest::TestAll();
	//CollisionManagerTest::TestAll();
	//MemberManagerTest::TestAll();

	PackageManagerTest::TestAll<true>();
	//PackageManagerTest::TestIncludeTypes<true>();

	//CppGeneratorTest::TestAll();

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Enum))
			continue;

		auto Info = EnumManager::GetInfo(Obj.Cast<UEEnum>());

		for (const auto& ValueInfo : Info.GetMemberCollisionInfoIterator())
		{
			if (ValueInfo.GetCollisionCount() > 9)
				std::cout << ValueInfo.GetUniqueName() << std::endl;
		}
	}


	//MemberManagerTest::TestFunctionIterator<true>();

	//CppGeneratorTest::TestAll();

	//for (int i = 0; i < 100; i++)
	//{
	//	for (auto Obj : ObjectArray())
	//	{
	//		Obj.GetFullName();
	//
	//		if (Obj.IsA(EClassCastFlags::Struct))
	//		{
	//			for (auto Prop : Obj.Cast<UEStruct>().GetProperties())
	//				Prop.GetName();
	//		}
	//	}
	//}
	
	//GeneratorRewrite::Generate<CppGenerator>();

	//std::cout << "FTransform::MinAlignment: " << *reinterpret_cast<int32*>(static_cast<uint8*>(ObjectArray::FindObjectFast("Transform", EClassCastFlags::Struct)) + Off::UStruct::Size + 0x4) << std::endl;
	//Generator::GenerateSDK();
	//Generator::GenerateMappings();
	//Generator::GenerateIDAMappings();

	auto t_C = high_resolution_clock::now();
	
	auto ms_int_ = duration_cast<milliseconds>(t_C - t_1);
	duration<double, std::milli> ms_double_ = t_C - t_1;
	
	std::cout << "\n\nGenerating SDK took (" << ms_double_.count() << "ms)\n\n\n";

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stdout);
			if (Dummy) fclose(Dummy);
			FreeConsole();

			FreeLibraryAndExitThread(Module, 0);
		}

		Sleep(100);
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
		break;
	}

	return TRUE;
}