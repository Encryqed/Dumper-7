#pragma once
#include "HashStringTable.h"
#include "EnumManager.h"
#include "TestBase.h"

#include <cassert>


class EnumManagerTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestInit<bDoDebugPrinting>();
		TestInfo<bDoDebugPrinting>();
		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInit()
	{
		EnumManager::Init();

		int32 FirstInitSize = EnumManager::EnumInfoOverrides.size();

		EnumManager::Init();
		EnumManager::Init();
		EnumManager::Init();

		bool bSuccededTestWithoutError = EnumManager::EnumInfoOverrides.size() == FirstInitSize;

		PrintDbgMessage<bDoDebugPrinting>("{} --> NameInfos.size(): 0x{:X} -> 0x{:X}", __FUNCTION__, FirstInitSize, EnumManager::EnumInfoOverrides.size());
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInfo()
	{
		EnumManager::Init();

		UEEnum ENetRole = ObjectArray::FindObjectFast<UEEnum>("ENetRole");
		UEEnum ETraceTypeQuery = ObjectArray::FindObjectFast<UEEnum>("ETraceTypeQuery");

		EnumInfoHandle NetRole = EnumManager::GetInfo(ENetRole);
		EnumInfoHandle TraceTypeQuery = EnumManager::GetInfo(ETraceTypeQuery);

#define EnumInfoHandleToDebugInfo(InfoHandle) \
	InfoHandle.GetName().GetName(), InfoHandle.GetName().IsUnique(), InfoHandle.GetUnderlyingTypeSize()

		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ UnderlayingSize=0x{:X} }}\n", EnumInfoHandleToDebugInfo(NetRole));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ UnderlayingSize=0x{:X} }}\n", EnumInfoHandleToDebugInfo(TraceTypeQuery));
		PrintDbgMessage<bDoDebugPrinting>("");

		bool bSuccededTestWithoutError = true;

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};