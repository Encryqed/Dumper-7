#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "Enums.h"
#include "ObjectArray.h"
#include "Utils.h"
#include "OffsetFinder.h"
#include "Offsets.h"
#include "Package.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

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

	ObjectArray::Init();
	FName::Init();
	Off::Init();
	

	std::cout << "Some FullName: " << ObjectArray::GetByIndex(69).GetFullName() << "\n";

	auto t_1 = high_resolution_clock::now();
	
	std::ofstream DumpStream("Properties.txt");

	DumpStream << "Properties:\n\n";

	for (auto Object : ObjectArray())
	{
		if (Object.IsA(EClassCastFlags::UProperty) && !Object.HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			DumpStream << Object.GetFullName() << "\n";
			DumpStream << Object.Cast<UEProperty>().GetCppType() << " " << Object.Cast<UEProperty>().GetValidName() << ";\n";
		}
	}
	DumpStream.close();

	auto t_2 = high_resolution_clock::now();

	
	std::vector<UEProperty> Properties;
	std::vector<Types::Member> Members;

	Package Pack(nullptr);

	UEClass FortPC = ObjectArray::FindClassFast("FortPlayerController");
	UEStruct FFortGameplayEffectContext = ObjectArray::FindObjectFast("FortGameplayEffectContext", EClassCastFlags::UScriptStruct).Cast<UEStruct>();
	UEEnum EFortToastType = ObjectArray::FindObjectFast("EFortToastType", EClassCastFlags::UEnum).Cast<UEEnum>();

	Types::Class Clss = Pack.GenerateClass(FortPC);
	Types::Struct Strct = Pack.GenerateStruct(FFortGameplayEffectContext);
	Types::Enum Enm = Pack.GenerateEnum(EFortToastType);

	auto ms_int_ = duration_cast<milliseconds>(t_2 - t_1);
	duration<double, std::milli> ms_double_ = t_2 - t_1;

	std::cout << "\n\Finding Offsets took (" << ms_int_.count() << "ms)\n\n\n";

	std::cout << "Some FullName: " << ObjectArray::GetByIndex(69).GetFullName() << "\n";

	std::cout << "\n" << Clss.GetGeneratedBody() << "\n\n\n\n";
	std::cout << "\n" << Strct.GetGeneratedBody() << "\n\n\n";
	std::cout << "\n" << Enm.GetGeneratedBody() << "\n\n";

	std::cout << "Body: NONEOENENENEOENEOENENENOEOENEO\n\n";

	{
		auto t_1 = high_resolution_clock::now();
		auto t_2 = high_resolution_clock::now();
	
		auto ms_int_ = duration_cast<milliseconds>(t_2 - t_1);
		duration<double, std::milli> ms_double_ = t_2 - t_1;
	
		std::cout << "\n\nComparing 0 times took (" << ms_int_.count() << "ms)\n\n\n";
	}

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stdout);
			fclose(Dummy);
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

