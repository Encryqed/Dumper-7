#pragma once
#include <vector>
#include "ObjectArray.h"

namespace OffsetFinder
{

	inline constexpr const int MaxOffset = 0x1A0;

	template<int Alignement = 4, typename T>
	inline int32_t FindOffset(std::vector<std::pair<void*, T>>& ObjectValuePair)
	{
		int32_t HighestFoundOffset = 0x28;

		for (int i = 0; i < ObjectValuePair.size(); i++)
		{
			uint8_t* BytePtr = (uint8_t*)(ObjectValuePair[i].first);

			for (int j = HighestFoundOffset; j < MaxOffset; j += Alignement)
			{
				if ((*(T*)(BytePtr + j)) == ObjectValuePair[i].second && j >= HighestFoundOffset)
				{
					if (j > HighestFoundOffset)
					{
						HighestFoundOffset = j;
						i = 0;
					}
					j = MaxOffset;
				}
			}
		}
		return HighestFoundOffset;
	}

	/* UEnum */
	inline int32_t FindEnumNamesOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("ENetRole").GetAddress(), 0x5 });
		Infos.push_back({ ObjectArray::FindObjectFast("EWindowTitleBarMode").GetAddress(), 0x3 });
		Infos.push_back({ ObjectArray::FindObjectFast("ETraceTypeQuery").GetAddress(), 0x22 });

		return FindOffset(Infos) - 0x8;
	}


	/* UStruct */
	inline int32_t FindSuperOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Field").GetAddress(), ObjectArray::FindObjectFast("Object").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Struct").GetAddress(), ObjectArray::FindObjectFast("Field").GetAddress() });
		
		//Thanks to the ue4 dev who decided UStruct should be spelled Ustruct
		if (Infos[1].first == nullptr)
			Infos[1].first = ObjectArray::FindObjectFast("struct").GetAddress();

		return FindOffset(Infos);
	}

	inline int32_t FindChildOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Vector").GetAddress(), ObjectArray::FindObject("FloatProperty CoreUObject.Vector.X").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector4").GetAddress(), ObjectArray::FindObject("FloatProperty CoreUObject.Vector4.X").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector2D").GetAddress(), ObjectArray::FindObject("FloatProperty CoreUObject.Vector2D.X").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Guid").GetAddress(), ObjectArray::FindObject("IntProperty CoreUObject.Guid.A").GetAddress() });

		return FindOffset(Infos);
	}

	inline int32_t FindStructSizeOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Vector2D").GetAddress(), 0x08 });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector").GetAddress(), 0x0C });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector4").GetAddress(), 0x10 });

		return FindOffset(Infos);
	}


	/* UFunction */
	inline int32_t FindFunctionFlagsOffset()
	{
		std::vector<std::pair<void*, EFunctionFlags>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("WasInputKeyJustPressed").GetAddress(), EFunctionFlags::Final | EFunctionFlags::Native | EFunctionFlags::Public | EFunctionFlags::BlueprintCallable | EFunctionFlags::BlueprintPure | EFunctionFlags::Const });
		Infos.push_back({ ObjectArray::FindObjectFast("ToggleSpeaking").GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });
		Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel").GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });
		Infos.push_back({ ObjectArray::FindObjectFast("StopHapticEffect").GetAddress(), EFunctionFlags::Final | EFunctionFlags::Native | EFunctionFlags::Public | EFunctionFlags::BlueprintCallable });

		return FindOffset(Infos);
	}

	inline int32_t FindNumParamsOffset()
	{
		std::vector<std::pair<void*, uint8_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel").GetAddress(), 0x1 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetViewTargetWithBlend").GetAddress(), 0x5 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetHapticsByValue").GetAddress(), 0x3 });
		Infos.push_back({ ObjectArray::FindObjectFast("SphereTraceSingleForObjects").GetAddress(), 0xE });

		return FindOffset<1>(Infos);
	}

	inline int32_t FindParamSizeOffset()
	{
		std::vector<std::pair<void*, uint16_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel").GetAddress(), 0x10 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetViewTargetWithBlend").GetAddress(), 0x15 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetHapticsByValue").GetAddress(), 0x9 });
		Infos.push_back({ ObjectArray::FindObjectFast("SphereTraceSingleForObjects").GetAddress(), 0x109 });

		return FindOffset<1>(Infos);
	}


	/* UClass */
	inline int32_t FindCastFlagsOffset()
	{
		std::vector<std::pair<void*, EClassCastFlags>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Actor").GetAddress(), EClassCastFlags::AActor });
		Infos.push_back({ ObjectArray::FindObjectFast("Class").GetAddress(), EClassCastFlags::UField | EClassCastFlags::UStruct | EClassCastFlags::UClass });
		
		return FindOffset(Infos);
	}

	inline int32_t FindDefaultObjectOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Object").GetAddress(), ObjectArray::FindObjectFast("Default__Object").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Field").GetAddress(), ObjectArray::FindObjectFast("Default__Field").GetAddress() });

		return FindOffset(Infos);
	}

	
	/* UProperty */
	inline int32_t FindElementSizeOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Default__Int8Property").GetAddress(), 0x1 });
		Infos.push_back({ ObjectArray::FindObjectFast("Default__Int16Property").GetAddress(), 0x2 });
		Infos.push_back({ ObjectArray::FindObjectFast("Default__IntProperty").GetAddress(), 0x4 });
		Infos.push_back({ ObjectArray::FindObjectFast("Default__Int64Property").GetAddress(), 0x8 });

		return FindOffset(Infos);
	}

	inline int32_t FindPropertyFlagsOffset()
	{
		std::vector<std::pair<void*, EPropertyFlags>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("HiddenActors").GetAddress(), EPropertyFlags::ZeroConstructor | EPropertyFlags::NativeAccessSpecifierPublic });
		Infos.push_back({ ObjectArray::FindObjectFast("TextureObject").GetAddress(), EPropertyFlags::NoDestructor | EPropertyFlags::NativeAccessSpecifierPublic });
		Infos.push_back({ ObjectArray::FindObjectFast("bActorEnableCollision").GetAddress(), EPropertyFlags::NoDestructor | EPropertyFlags::HasGetValueTypeHash | EPropertyFlags::NativeAccessSpecifierPrivate });

		return FindOffset(Infos);
	}

	inline int32_t FindOffsetInternalOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("PrimaryActorTick").GetAddress(), Off::UObject::Outer + 0x8 });
		Infos.push_back({ ObjectArray::FindObjectFast("DebugCameraControllerClass").GetAddress(), Off::UObject::Outer + 0x10 });

		return FindOffset(Infos);
	}


	/* UByteProperty */
	inline int32_t FindEnumOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		void* ENetRoleObject = ObjectArray::FindObjectFast("ENetRole").GetAddress();
		Infos.push_back({ ObjectArray::FindObjectFast("RemoteRole").GetAddress(), ENetRoleObject });
		Infos.push_back({ ObjectArray::FindObjectFast("Role").GetAddress(), ENetRoleObject });

		return FindOffset(Infos);
	}


	/* UBoolProperty */
	inline int32_t FindBoolPropertyBaseOffset()
	{
		std::vector<std::pair<void*, uint8_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("bReplicatedHasBegunPlay").GetAddress(), 0xFF });
		Infos.push_back({ ObjectArray::FindObjectFast("bRequiresPushToTalk").GetAddress(), 0xFF });

		return (FindOffset<1>(Infos) - 0x3);
	}


	/* UObjectPropertyBase */
	inline int32_t FindPropertyClassOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;
	
		Infos.push_back({ ObjectArray::FindObjectFast("AuthorityGameMode").GetAddress(), ObjectArray::FindObjectFast("GameModeBase").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("NetworkManager").GetAddress(), ObjectArray::FindObjectFast("GameNetworkManager").GetAddress() });
	
		return FindOffset(Infos);
	}


	/* UClassProperty */
	inline int32_t FindMetaClassOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("GameSessionClass").GetAddress(), ObjectArray::FindObjectFast("GameSession").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("GameStateClass").GetAddress(), ObjectArray::FindObjectFast("GameStateBase").GetAddress() });

		return FindOffset(Infos);
	}
	

	/* UScriptProperty */
	// Mabybe later


	/* UWeakObjectProperty */


	/* UStructProperty */
	inline int32_t FindStructTypeOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		void* VectorStruct = ObjectArray::FindObjectFast("Vector").GetAddress();
		Infos.push_back({ ObjectArray::FindObjectFast("Scale3D").GetAddress(),VectorStruct });
		Infos.push_back({ ObjectArray::FindObjectFast("Translation").GetAddress(), VectorStruct });

		return FindOffset(Infos);
	}


	/* UArrayProperty */
	inline int32_t FindInnerTypeOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("PlayerArray").GetAddress(), ObjectArray::FindObject("ObjectProperty Engine.GameStateBase.PlayerArray.PlayerArray").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("ClickEventKeys").GetAddress(), ObjectArray::FindObject("StructProperty Engine.PlayerController.ClickEventKeys.ClickEventKeys").GetAddress() });

		return FindOffset(Infos);
	}


	/* UMapProperty */
	inline int32_t FindMapPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("ProfileTable").GetAddress(), ObjectArray::FindObjectFast("ProfileTable_Key").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("CurrentCategorizedQuestsMap").GetAddress(), ObjectArray::FindObjectFast("CurrentCategorizedQuestsMap_Key").GetAddress() });

		return FindOffset(Infos);
	}


	/* USetProperty */
	inline int32_t FindSetPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Proxies").GetAddress(), ObjectArray::FindObject("ObjectProperty Landscape.LandscapeInfo.Proxies.Proxies").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Levels").GetAddress(), ObjectArray::FindObject("ObjectProperty Engine.LevelCollection.Levels.Levels").GetAddress() });

		return FindOffset(Infos);
	}


	/* UEnumProperty */
	inline int32_t FindEnumPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("RuntimeGeneration").GetAddress(), ObjectArray::FindObject("ByteProperty NavigationSystem.NavigationData.RuntimeGeneration.UnderlyingType").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("AutoPossessAI").GetAddress(), ObjectArray::FindObject("ByteProperty Engine.Pawn.AutoPossessAI.UnderlyingType").GetAddress() });

		return FindOffset(Infos);
	}

}