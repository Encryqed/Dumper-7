#pragma once
#include "HashStringTable.h"
#include "StructManager.h"
#include "TestBase.h"

#include <cassert>


class StructManagerTest : protected TestBase
{
public:
	template<bool bDoDebugPrinting = false>
	static inline void TestAll()
	{
		TestInit<bDoDebugPrinting>();
		TestInfo<bDoDebugPrinting>();
		TestIsFinal<bDoDebugPrinting>();
		PrintDbgMessage<bDoDebugPrinting>("");
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInit()
	{
		StructManager::Init();

		int32 FirstInitSize = StructManager::StructInfoOverrides.size();

		StructManager::Init();
		StructManager::Init();
		StructManager::Init();

		bool bSuccededTestWithoutError = StructManager::StructInfoOverrides.size() == FirstInitSize;

		PrintDbgMessage<bDoDebugPrinting>("{} --> NameInfos.size(): 0x{:X} -> 0x{:X}", __FUNCTION__, FirstInitSize, StructManager::StructInfoOverrides.size());
		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestInfo()
	{
		StructManager::Init();

		UEStruct AActor = ObjectArray::FindClassFast("Actor");
		UEStruct UObject = ObjectArray::FindClassFast("Object");
		UEStruct UEngine = ObjectArray::FindClassFast("Engine");
		UEStruct FVector = ObjectArray::FindObjectFast<UEStruct>("Vector");
		UEStruct FTransform = ObjectArray::FindObjectFast<UEStruct>("Transform");
		UEStruct FQuat = ObjectArray::FindObjectFast<UEStruct>("Quat");
		UEStruct UKismetSystemLibrary = ObjectArray::FindClassFast("KismetSystemLibrary");
		UEStruct ULevelStreaming = ObjectArray::FindClassFast("LevelStreaming");
		//UEStruct FPrimitiveComponentInstanceData = ObjectArray::FindObjectFast<UEStruct>("PrimitiveComponentInstanceData");
		//UEStruct FStaticMeshComponentInstanceData = ObjectArray::FindObjectFast<UEStruct>("StaticMeshComponentInstanceData");
		//UEStruct UFortAIHotSpotSlot = ObjectArray::FindClassFast("FortAIHotSpotSlot");
		//UEStruct UAIHotSpotSlot = ObjectArray::FindClassFast("AIHotSpotSlot");

		StructInfoHandle ActorInfo = StructManager::GetInfo(AActor);
		StructInfoHandle ObjectInfo = StructManager::GetInfo(UObject);
		StructInfoHandle EngineInfo = StructManager::GetInfo(UEngine);
		StructInfoHandle VectorInfo = StructManager::GetInfo(FVector);
		StructInfoHandle TransformInfo = StructManager::GetInfo(FTransform);
		StructInfoHandle QuatInfo = StructManager::GetInfo(FQuat);
		StructInfoHandle KismetSystemLibraryInfo = StructManager::GetInfo(UKismetSystemLibrary);
		StructInfoHandle LevelStreamingInfo = StructManager::GetInfo(ULevelStreaming);
		//StructInfoHandle PrimitiveComponentInstanceData = StructInfos.GetInfo(FPrimitiveComponentInstanceData);
		//StructInfoHandle StaticMeshComponentInstanceData = StructInfos.GetInfo(FStaticMeshComponentInstanceData);
		//StructInfoHandle FortAIHotSpotSlot = StructInfos.GetInfo(UFortAIHotSpotSlot);
		//StructInfoHandle AIHotSpotSlot = StructInfos.GetInfo(UAIHotSpotSlot);

#define StructInfoHandleToDebugInfo(InfoHandle) \
	InfoHandle.GetName().GetName(), InfoHandle.GetName().IsUnique(), InfoHandle.GetSize(), InfoHandle.GetAlignment(), InfoHandle.ShouldUseExplicitAlignment(), InfoHandle.IsFinal()

		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(ActorInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(ObjectInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(EngineInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(VectorInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(TransformInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(QuatInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(KismetSystemLibraryInfo));
		PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(LevelStreamingInfo));
		//PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(PrimitiveComponentInstanceData));
		//PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(StaticMeshComponentInstanceData));
		//PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(FortAIHotSpotSlot));
		//PrintDbgMessage<bDoDebugPrinting>("{}[{}]: {{ Size=0x{:X}, Alignment=0x{:X}, bUseExplicitAlignment={}, bIsFinal={} }}\n", StructInfoHandleToDebugInfo(AIHotSpotSlot));
		PrintDbgMessage<bDoDebugPrinting>("");

		bool bSuccededTestWithoutError = true;

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}

	template<bool bDoDebugPrinting = false>
	static inline void TestIsFinal()
	{
		StructManager::Init();

		bool bSuccededTestWithoutError = true;
		
		std::unordered_set<int32> AllSupers;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			if (UEStruct Super = Obj.Cast<UEStruct>().GetSuper())
				AllSupers.insert(Super.GetIndex());
		}

		auto SupersEnd = AllSupers.end();

		for (auto& [StructIdx, Info] : StructManager::StructInfoOverrides)
		{
			if (!Info.bIsFinal)
				continue;

			if (AllSupers.find(StructIdx) != SupersEnd)
			{
				PrintDbgMessage<bDoDebugPrinting>("Struct '{}' was incorrectly marked as 'final'.", ObjectArray::GetByIndex(StructIdx).GetCppName());
				SetBoolIfFailed(bSuccededTestWithoutError, false);
				break;
			}
		}

		std::cout << __FUNCTION__ << ": " << (bSuccededTestWithoutError ? "SUCCEEDED!" : "FAILED!") << std::endl;
	}
};