#pragma once

#include <vector>
#include "ObjectArray.h"

namespace OffsetFinder
{
	template<int Alignement = 4, typename T, bool bStartAtZero = false>
	inline int32_t FindOffset(std::vector<std::pair<void*, T>>& ObjectValuePair, int MaxOffset = 0x1A0)
	{
		int32_t HighestFoundOffset = bStartAtZero ? 0x0 : 0x28;

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


	inline int32_t GetValidPointerOffset(uint8_t* ObjA, uint8_t* ObjB, int32_t StartingOffset, int32_t MaxOffset)
	{
		for (int j = StartingOffset; j <= MaxOffset; j += 0x8)
		{
			if (!IsBadReadPtr(*(void**)(ObjA + j)) && !IsBadReadPtr(*(void**)(ObjB + j)))
			{
				return j;
			}
		}

		return -1;
	};

	/* UObject */
	inline void InitUObjectOffsets()
	{
		uint8_t* ObjA = (uint8_t*)ObjectArray::GetByIndex(0x55).GetAddress();
		uint8_t* ObjB = (uint8_t*)ObjectArray::GetByIndex(0x123).GetAddress();

		auto GetIndexOffset = [&ObjA, &ObjB]() -> int32_t
		{
			std::vector<std::pair<void*, int32_t>> Infos;

			Infos.emplace_back(ObjectArray::GetByIndex(0x055).GetAddress(), 0x055);
			Infos.emplace_back(ObjectArray::GetByIndex(0x123).GetAddress(), 0x123);

			return FindOffset<4, int32_t, true>(Infos);
		};

		Off::UObject::Vft = 0x00;
		Off::UObject::Flags = sizeof(void*);
		Off::UObject::Index = GetIndexOffset();
		Off::UObject::Class = GetValidPointerOffset(ObjA, ObjB, Off::UObject::Index + sizeof(int), 0x40);
		Off::UObject::Name = Off::UObject::Class + sizeof(void*);
		Off::UObject::Outer = GetValidPointerOffset(ObjA, ObjB, Off::UObject::Name + 0x8, 0x40);

		// loop a few times in case we accidentally choose a UPackage (which doesn't have an Outer) to find Outer
		while (Off::UObject::Outer == -1)
		{
			ObjA = (uint8*)ObjectArray::GetByIndex(rand() % 0x400).GetAddress();
			ObjB = (uint8*)ObjectArray::GetByIndex(rand() % 0x400).GetAddress();

			Off::UObject::Outer = GetValidPointerOffset(ObjA, ObjB, Off::UObject::Name + 0x8, 0x40);
		}

		Off::InSDK::FNameSize = Off::UObject::Outer - Off::UObject::Name;

		if (Off::InSDK::FNameSize > 0x8)
		{
			Settings::Internal::bUseCasePreservingName = true;

			Off::FName::CompIdx = 0x0;
			Off::FName::Number = 0x8;
		}
		else
		{
			Settings::Internal::bUseCasePreservingName = false;

			Off::FName::CompIdx = 0x0;
			Off::FName::Number = 0x4;
		}
	}

	inline void FixupHardcodedOffsets()
	{
		if (Settings::Internal::bUseCasePreservingName)
		{
			Off::FField::Flags += 0x8;

			Off::FFieldClass::Id += 0x08;
			Off::FFieldClass::CastFlags += 0x08;
			Off::FFieldClass::ClassFlags += 0x08;
			Off::FFieldClass::SuperClass += 0x08;
		}

		if (Settings::Internal::bUseFProperty)
		{
			/*
			* On versions below 5.1.1: class FFieldVariant { void*, bool } -> extends to { void*, bool, uint8[0x7] }
			* ON versions since 5.1.1: class FFieldVariant { void* }
			* 
			* Check: 
			* if FFieldVariant contains a bool, the memory at the bools offset will not be a valid pointer
			* if FFieldVariant doesn't contain a bool, the memory at the bools offset will be the next member of FField, the Next ptr [valid]
			*/

			void* PossibleNextPtrOrBool0 = *(void**)((uint8*)ObjectArray::FindClassFast("Actor").GetChildProperties().GetAddress() + 0x18);
			void* PossibleNextPtrOrBool1 = *(void**)((uint8*)ObjectArray::FindClassFast("ActorComponent").GetChildProperties().GetAddress() + 0x18);
			void* PossibleNextPtrOrBool2 = *(void**)((uint8*)ObjectArray::FindClassFast("Pawn").GetChildProperties().GetAddress() + 0x18);

			auto IsValidPtr = [](void* a) -> bool
			{
				return !IsBadReadPtr(a) && (uintptr_t(a) & 0x1) == 0; // realistically, there wont be any pointers to unaligned memory
			};

			if (IsValidPtr(PossibleNextPtrOrBool0) && IsValidPtr(PossibleNextPtrOrBool1) && IsValidPtr(PossibleNextPtrOrBool2))
			{
				std::cout << "Applaying fix to hardcoded offsets \n" << std::endl;

				Settings::Internal::bUseMaskForFieldOwner = true;

				Off::FField::Next -= 0x08;
				Off::FField::Name -= 0x08;
				Off::FField::Flags -= 0x08;
			}
		}
	}

	/* UEnum */
	inline int32_t FindFieldNextOffset()
	{
		return Off::UObject::Outer + 0x8;
	}

	/* UEnum */
	inline int32_t FindEnumNamesOffset()
	{
		std::vector<std::pair<void*, int32_t>> Infos;

		Infos.push_back({ ObjectArray::FindObjectFast("ENetRole").GetAddress(), 0x5 });			
		Infos.push_back({ ObjectArray::FindObjectFast("ETraceTypeQuery").GetAddress(), 0x22 }); 

		int Ret = FindOffset(Infos) - 0x8;

		struct Name08Byte { uint8 Pad[0x08]; };
		struct Name16Byte { uint8 Pad[0x10]; };

		if (Settings::Internal::bUseCasePreservingName)
		{
			if (reinterpret_cast<TArray<TPair<Name16Byte, int64>>*>(static_cast<uint8*>(Infos[0].first) + Ret)->operator[](1).Second != 1)
				Settings::Internal::bIsEnumNameOnly = true;
		}
		else
		{
			if (reinterpret_cast<TArray<TPair<Name08Byte, int64>>*>(static_cast<uint8*>(Infos[0].first) + Ret)->operator[](1).Second != 1)
				Settings::Internal::bIsEnumNameOnly = true;
		}

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
		uint8* ObjA = (uint8*)ObjectArray::FindObjectFast("Color").GetAddress();
		uint8* ObjB = (uint8*)ObjectArray::FindObjectFast("Guid").GetAddress();

		return GetValidPointerOffset(ObjA, ObjB, Off::UStruct::Children + 0x08, 0x80);
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

		UEStruct Guid = ObjectArray::FindObjectFast("Guid", EClassCastFlags::Struct).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Guid, "A").GetAddress(), 0x04 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Guid, "B").GetAddress(), 0x04 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Guid, "C").GetAddress(), 0x04 });

		return FindOffset(Infos);
	}

	inline int32_t FindPropertyFlagsOffset()
	{
		std::vector<std::pair<void*, EPropertyFlags>> Infos;

		UEStruct Actor = ObjectArray::FindObjectFast("Actor", EClassCastFlags::Class).Cast<UEStruct>();
		UEStruct PlayerController = ObjectArray::FindObjectFast("PlayerController", EClassCastFlags::Class).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Actor, "bActorEnableCollision").GetAddress(), EPropertyFlags::NoDestructor | EPropertyFlags::HasGetValueTypeHash | EPropertyFlags::NativeAccessSpecifierPrivate });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(PlayerController, "HiddenActors").GetAddress(), EPropertyFlags::ZeroConstructor | EPropertyFlags::NativeAccessSpecifierPublic });

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

		UEStruct Color = ObjectArray::FindObjectFast("Color", EClassCastFlags::Struct).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Color, "B").GetAddress(), 0x00 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Color, "G").GetAddress(), 0x01 });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Color, "R").GetAddress(), 0x02 });

		return FindOffset(Infos);
	}

	/* UByteProperty */
	inline int32_t FindEnumOffset()
	{
		std::vector<std::pair<void*, void*>> Infos;

		void* ENetRoleObject = ObjectArray::FindObjectFast("ENetRole").GetAddress();
		UEStruct Actor = ObjectArray::FindObjectFast("Actor", EClassCastFlags::Class).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Actor, "RemoteRole").GetAddress(), ENetRoleObject });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Actor, "Role").GetAddress(), ENetRoleObject });

		return FindOffset(Infos);
	}

	/* UBoolProperty */
	inline int32_t FindBoolPropertyBaseOffset()
	{
		std::vector<std::pair<void*, uint8_t>> Infos;

		UEStruct GameStateBase = ObjectArray::FindObjectFast("GameStateBase", EClassCastFlags::Class).Cast<UEStruct>();
		UEStruct PlayerController = ObjectArray::FindObjectFast("PlayerController", EClassCastFlags::Class).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(GameStateBase, "bReplicatedHasBegunPlay").GetAddress(), 0xFF });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(PlayerController, "bAutoManageActiveCameraTarget").GetAddress(), 0xFF });

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

		UEStruct GameModeBase = ObjectArray::FindObjectFast("GameModeBase", EClassCastFlags::Class).Cast<UEStruct>();

		Infos.push_back({ ObjectArray::FindMemberInObjectFast(GameModeBase, "GameSessionClass").GetAddress(), ObjectArray::FindObjectFast("GameSession").GetAddress() });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(GameModeBase, "GameStateClass").GetAddress(), ObjectArray::FindObjectFast("GameStateBase").GetAddress() });

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

		UEStruct Transform = ObjectArray::FindObjectFast("Transform", EClassCastFlags::Struct).Cast<UEStruct>();

		void* VectorStruct = ObjectArray::FindObjectFast("Vector").GetAddress();
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Transform, "Scale3D").GetAddress(), VectorStruct });
		Infos.push_back({ ObjectArray::FindMemberInObjectFast(Transform, "Translation").GetAddress(), VectorStruct });

		return FindOffset(Infos);
	}

	/* UArrayProperty */
	inline int32_t FindInnerTypeOffset(const int32 PropertySize)
	{
		if (!Settings::Internal::bUseFProperty)
			return PropertySize;

		if (auto Object = ObjectArray::FindMemberInObjectFast(ObjectArray::FindClassFast("GameViewportClient"), "DebugProperties", EClassCastFlags::ArrayProperty))
		{
			void* AddressToCheck = *(void**)((uint8*)Object.GetAddress() + PropertySize);

			if (IsBadReadPtr(AddressToCheck))
				return PropertySize + 0x8;
		}

		return PropertySize;
	}

	/* USetProperty */
	inline int32_t FindSetPropertyBaseOffset(const int32 PropertySize)
	{
		if (!Settings::Internal::bUseFProperty)
			return PropertySize;

		if (auto Object = ObjectArray::FindMemberInObjectFast(ObjectArray::FindClassFast("InputDeviceSubsystem"), "ActiveProperties", EClassCastFlags::SetProperty))
		{
			void* AddressToCheck = *(void**)((uint8*)Object.GetAddress() + PropertySize);

			if (IsBadReadPtr(AddressToCheck))
				return PropertySize + 0x8;
		}

		return PropertySize;
	}

	/* UMapProperty */
	inline int32_t FindMapPropertyBaseOffset(const int32 PropertySize)
	{
		if (!Settings::Internal::bUseFProperty)
			return PropertySize;

		if (auto Object = ObjectArray::FindMemberInObjectFast(ObjectArray::FindClassFast("ActorContainer"), "Actors", EClassCastFlags::MapProperty))
		{
			void* AddressToCheck = *(void**)((uint8*)Object.GetAddress() + PropertySize);

			if (IsBadReadPtr(AddressToCheck))
				return PropertySize + 0x8;
		}

		return PropertySize;
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
