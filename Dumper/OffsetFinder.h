#pragma once

#include <vector>
#include "ObjectArray.h"

namespace OffsetFinder
{
	template<int Alignement = 4, typename T>
	inline int32_t FindOffset(std::vector<std::pair<void*, T>>& ObjectValuePair, int MaxOffset = 0x1A0)
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
		Infos.push_back({ ObjectArray::FindObjectFast("ETraceTypeQuery").GetAddress(), 0x22 }); 

		int Ret = FindOffset(Infos) - 0x8;

		if (reinterpret_cast<TArray<TPair<FName, int64>>*>(static_cast<uint8*>(Infos[0].first) + Ret)->operator[](1).Second != 1)
			Settings::Internal::bIsEnumNameOnly = true;

		return Ret;
	}

	/* UStruct */
	inline int32_t FindSuperOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Struct").GetAddress(), ObjectArray::FindObjectFast("Field").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Class").GetAddress(), ObjectArray::FindObjectFast("Struct").GetAddress() });
		
		// Thanks to the ue4 dev who decided UStruct should be spelled Ustruct
		if (Infos[0].first == nullptr)
			Infos[0].first = Infos[1].second = ObjectArray::FindObjectFast("struct").GetAddress();

		return FindOffset(Infos);
	}

	inline int32_t FindChildOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("PlayerController").GetAddress(), ObjectArray::FindObjectFastInOuter("WasInputKeyJustReleased", "PlayerController").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Controller").GetAddress(), ObjectArray::FindObjectFastInOuter("UnPossess", "Controller").GetAddress() });

		if (FindOffset(Infos) == 0x28)
		{
			Infos.clear();

			Infos.push_back({ ObjectArray::FindObjectFast("Vector").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector").GetAddress() });
			Infos.push_back({ ObjectArray::FindObjectFast("Vector4").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector4").GetAddress() });
			Infos.push_back({ ObjectArray::FindObjectFast("Vector2D").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector2D").GetAddress() });
			Infos.push_back({ ObjectArray::FindObjectFast("Guid").GetAddress(), ObjectArray::FindObjectFastInOuter("A","Guid").GetAddress() });

			return FindOffset(Infos);
		}

		Settings::Internal::bUseFProperty = true;

		return FindOffset(Infos);
	}

	inline int32_t FindChildPropertiesOffset()
	{
		// TODO (encryqed) : Get Valid ChildProperties Offset for UStruct :)
		
		return Off::UStruct::Children + 0x08;
	}

	inline int32_t FindStructSizeOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Color").GetAddress(), 0x04 });
		Infos.push_back({ ObjectArray::FindObjectFast("Guid").GetAddress(), 0x10 });

		return FindOffset(Infos);
	}

	/* UFunction */
	inline int32_t FindFunctionFlagsOffset()
	{
		std::vector<std::pair<void*, EFunctionFlags>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("WasInputKeyJustPressed").GetAddress(), EFunctionFlags::Final | EFunctionFlags::Native | EFunctionFlags::Public | EFunctionFlags::BlueprintCallable | EFunctionFlags::BlueprintPure | EFunctionFlags::Const });
		Infos.push_back({ ObjectArray::FindObjectFast("ToggleSpeaking").GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });
		Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel").GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });
		
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

		// TODO (encryqed) : Fix it anyways somehow, for some reason its one byte off? Idk why i just add a byte manually
		Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel").GetAddress(), 0x10 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetViewTargetWithBlend").GetAddress(), 0x15 });
		Infos.push_back({ ObjectArray::FindObjectFast("SetHapticsByValue").GetAddress(), 0x9 });
		Infos.push_back({ ObjectArray::FindObjectFast("SphereTraceSingleForObjects").GetAddress(), 0x109 });

		int ParamSizeOffset = FindOffset<1>(Infos);

		if (Settings::Internal::bUseFProperty)
		{
			return ParamSizeOffset + 0x1;
		}

		return ParamSizeOffset;
	}

	/* UClass */
	inline int32_t FindCastFlagsOffset()
	{
		std::vector<std::pair<void*, EClassCastFlags>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("Actor").GetAddress(), EClassCastFlags::Actor });
		Infos.push_back({ ObjectArray::FindObjectFast("Class").GetAddress(), EClassCastFlags::Field | EClassCastFlags::Struct | EClassCastFlags::Class });
		
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

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Guid", "A").GetAddress(), 0x04 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Guid", "B").GetAddress(), 0x04 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Guid", "C").GetAddress(), 0x04 });

		//Infos.push_back({ ObjectArray::FindMemberInObjectFast("Vector", "X").GetAddress(), 0x4 });
		//Infos.push_back({ ObjectArray::FindMemberInObjectFast("Vector", "Y").GetAddress(), 0x4 });
		//
		//int Offset = FindOffset(Infos);
		//
		//if (Offset == 0x28)
		//{
		//	Infos.clear();
		//	Infos.push_back({ ObjectArray::FindMemberInObjectFast("Vector", "X").GetAddress(), 0x8 });
		//	Infos.push_back({ ObjectArray::FindMemberInObjectFast("Vector", "Y").GetAddress(), 0x8 });
		//
		//	Offset = FindOffset(Infos);
		//}
		//
		//return Offset;

		return FindOffset(Infos);
	}

	inline int32_t FindPropertyFlagsOffset()
	{
		std::vector<std::pair<void*, EPropertyFlags>> Infos;

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Actor", "bActorEnableCollision").GetAddress(), EPropertyFlags::NoDestructor | EPropertyFlags::HasGetValueTypeHash | EPropertyFlags::NativeAccessSpecifierPrivate });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("PlayerController", "HiddenActors").GetAddress(), EPropertyFlags::ZeroConstructor | EPropertyFlags::NativeAccessSpecifierPublic });

		int FlagsOffset = FindOffset(Infos);

		// Same flags without AccessSpecifier or Hashing flags
		if (FlagsOffset == 0x28)
		{
			Infos[1].second = EPropertyFlags::ZeroConstructor;
			Infos[0].second = EPropertyFlags::NoDestructor;

			FlagsOffset = FindOffset(Infos);
		}

		return FlagsOffset;
	}

	inline int32_t FindOffsetInternalOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Color", "B").GetAddress(), 0x00 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Color", "G").GetAddress(), 0x01 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Color", "R").GetAddress(), 0x02 });

		return FindOffset(Infos);
	}

	/* UByteProperty */
	inline int32_t FindEnumOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		void* ENetRoleObject = ObjectArray::FindObjectFast("ENetRole").GetAddress();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Actor", "RemoteRole").GetAddress(), ENetRoleObject });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Actor", "Role").GetAddress(), ENetRoleObject });

		return FindOffset(Infos);
	}

	/* UBoolProperty */
	inline int32_t FindBoolPropertyBaseOffset()
	{
		std::vector<std::pair<void*, uint8_t>> Infos;

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("GameStateBase", "bReplicatedHasBegunPlay").GetAddress(), 0xFF });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("PlayerController", "bAutoManageActiveCameraTarget").GetAddress(), 0xFF });

		return (FindOffset<1>(Infos) - 0x3);
	}

	/* UObjectPropertyBase */
	inline int32_t FindPropertyClassOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		// TODO (encryqed) : Gonna fix it later 
		Infos.push_back({ ObjectArray::FindObjectFast("AuthorityGameMode").GetAddress(), ObjectArray::FindObjectFast("GameModeBase").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("NetworkManager").GetAddress(), ObjectArray::FindObjectFast("GameNetworkManager").GetAddress() });
	
		return FindOffset(Infos);
	}

	/* UClassProperty */
	inline int32_t FindMetaClassOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		Infos.push_back({ ObjectArray::FindMemberInObjectFast("GameModeBase", "GameSessionClass").GetAddress(), ObjectArray::FindObjectFast("GameSession").GetAddress() });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("GameModeBase", "GameStateClass").GetAddress(), ObjectArray::FindObjectFast("GameStateBase").GetAddress() });

		return FindOffset(Infos);
	}
	
	/* UScriptProperty */
	// Mabybe later

	/* UWeakObjectProperty */
	// idk later

	/* UStructProperty */
	inline int32_t FindStructTypeOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		void* VectorStruct = ObjectArray::FindObjectFast("Vector").GetAddress();
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Transform", "Scale3D").GetAddress(), VectorStruct });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast("Transform", "Translation").GetAddress(), VectorStruct });

		return FindOffset(Infos);
	}

	/* UArrayProperty */
	inline int32_t FindInnerTypeOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		// TODO (encryqed) : Gonna fix it later 
		Infos.push_back({ ObjectArray::FindObjectFast("DebugProperties").GetAddress(), ObjectArray::FindObjectFastInOuter("DebugProperties", "DebugProperties").GetAddress()});
		Infos.push_back({ ObjectArray::FindObjectFast("ComponentTags").GetAddress(), ObjectArray::FindObjectFastInOuter("ComponentTags", "ComponentTags").GetAddress()});

		return FindOffset(Infos);
	}

	/* UMapProperty */
	inline int32_t FindMapPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		// TODO (encryqed) : Gonna fix it later 
		Infos.push_back({ ObjectArray::FindObjectFast("ProfileTable").GetAddress(), ObjectArray::FindObjectFast("ProfileTable_Key").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("CurrentCategorizedQuestsMap").GetAddress(), ObjectArray::FindObjectFast("CurrentCategorizedQuestsMap_Key").GetAddress() });

		return FindOffset(Infos);
	}

	/* USetProperty */
	inline int32_t FindSetPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		// TODO (encryqed) : Gonna fix it later 
		Infos.push_back({ ObjectArray::FindObjectFast("Proxies").GetAddress(), ObjectArray::FindObjectFastInOuter("Proxies", "Proxies").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Levels").GetAddress(), ObjectArray::FindObjectFastInOuter("Levels", "Levels").GetAddress()});

		return FindOffset(Infos);
	}

	/* UEnumProperty */
	inline int32_t FindEnumPropertyBaseOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		// TODO (encryqed) : Gonna fix it later 
		Infos.push_back({ ObjectArray::FindObjectFast("RuntimeGeneration").GetAddress(), ObjectArray::FindObjectFastInOuter("UnderlyingType", "RuntimeGeneration").GetAddress()});
		Infos.push_back({ ObjectArray::FindObjectFast("AutoPossessAI").GetAddress(), ObjectArray::FindObjectFastInOuter("UnderlyingType", "AutoPossessAI").GetAddress()});

		return FindOffset(Infos);
	}
}
