#include <vector>

#include "OffsetFinder/OffsetFinder.h"
#include "Unreal/ObjectArray.h"

/* UObject */
int32_t OffsetFinder::FindUObjectFlagsOffset()
{
	constexpr auto EnumFlagValueToSearch = 0x43;

	/* We're looking for a commonly occuring flag and this number basically defines the minimum number that counts ad "commonly occuring". */
	constexpr auto MinNumFlagValuesRequiredAtOffset = 0xA0;

	for (int i = 0; i < 0x20; i++)
	{
		int Offset = 0x0;
		while (Offset != OffsetNotFound)
		{
			// Look for 0x43 in this object, as it is a really common value for UObject::Flags
			Offset = FindOffset(std::vector{ std::pair{ ObjectArray::GetByIndex(i).GetAddress(), EnumFlagValueToSearch } }, Offset, 0x40);

			if (Offset == OffsetNotFound)
				break; // Early exit

			/* We're looking for a common flag. To check if the flag  is common we're checking the first 0x100 objects to see how often the flag occures at this offset. */
			int32 NumObjectsWithFlagAtOffset = 0x0;

			int Counter = 0;
			for (UEObject Obj : ObjectArray())
			{
				// Only check the (possible) flags of the first 0x100 objects
				if (Counter++ == 0x100)
					break;

				const int32 TypedValueAtOffset = *reinterpret_cast<int32*>(reinterpret_cast<uintptr_t>(Obj.GetAddress()) + Offset);

				if (TypedValueAtOffset == EnumFlagValueToSearch)
					NumObjectsWithFlagAtOffset++;
			}

			if (NumObjectsWithFlagAtOffset > MinNumFlagValuesRequiredAtOffset)
				return Offset;
		}
	}

	return OffsetNotFound;
}

int32_t OffsetFinder::FindUObjectIndexOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	Infos.emplace_back(ObjectArray::GetByIndex(0x055).GetAddress(), 0x055);
	Infos.emplace_back(ObjectArray::GetByIndex(0x123).GetAddress(), 0x123);

	return FindOffset<4>(Infos, sizeof(void*)); // Skip VTable
}

int32_t OffsetFinder::FindUObjectClassOffset()
{
	/* Checks for a pointer that points to itself in the end. The UObject::Class pointer of "Class CoreUObject.Class" will point to "Class CoreUObject.Class". */
	auto IsValidCyclicUClassPtrOffset = [](const uint8_t* ObjA, const uint8_t* ObjB, int32_t ClassPtrOffset)
	{
		/* Will be advanced before they are used. */
		const uint8_t* NextClassA = ObjA;
		const uint8_t* NextClassB = ObjB;

		for (int MaxLoopCount = 0; MaxLoopCount < 0x10; MaxLoopCount++)
		{
			const uint8_t* CurrentClassA = NextClassA;
			const uint8_t* CurrentClassB = NextClassB;

			NextClassA = *reinterpret_cast<const uint8_t* const*>(NextClassA + ClassPtrOffset);
			NextClassB = *reinterpret_cast<const uint8_t* const*>(NextClassB + ClassPtrOffset);

			/* If this was UObject::Class it would never be invalid. The pointer would simply point to itself.*/
			if (!NextClassA || !NextClassB || IsBadReadPtr(NextClassA) || IsBadReadPtr(NextClassB))
				return false;

			if (CurrentClassA == NextClassA && CurrentClassB == NextClassB)
				return true;
		}
	};

	const uint8_t* const ObjA = static_cast<const uint8_t*>(ObjectArray::GetByIndex(0x055).GetAddress());
	const uint8_t* const ObjB = static_cast<const uint8_t*>(ObjectArray::GetByIndex(0x123).GetAddress());

	int32_t Offset = 0;
	while (Offset != OffsetNotFound)
	{
		Offset = GetValidPointerOffset<true>(ObjA, ObjB, Offset + sizeof(void*), 0x50);

		if (IsValidCyclicUClassPtrOffset(ObjA, ObjB, Offset))
			return Offset;
	}

	return OffsetNotFound;
}

/*
* IsPotentialValidOffset: A function to filter offsets that can not possibly be valid for UObject::Name or FField::Name.
*						  Example for UObject::Name: it can 100% not be at the same offset as UObject::Class
* 
* DataGatherer: A function to gather values at the offsets not filterd by 'IsPotentialValidOffset'. Data is later used to filter more offsets, until hopefully only one is left.
*/
template<typename IteratorType>
int32_t FindNameOffsetForSomeClass(std::function<bool(int32_t Value)> IsPotentialValidOffset, IteratorType DataSetStartIterator, IteratorType DataSetEndIterator)
{
	/*
	* Requirements:
	*	- CmpIdx > 0x10 && CmpIdx < 0xF0000000
	*	- AverageValue >= 0x100 && AverageValue <= 0xFF00000;
	*	- Offset != { OtherOffsets }
	*/

	/* A struct describing the value */
	struct ValueInfo
	{
		int32 Offset;					   // Offset from the UObject start to this value
		int32 NumNamesWithLowCmpIdx = 0x0; // The number of names where the comparison index is in the range [0, 16]. Usually this should be far less than 0x20 names.
		uint64 TotalValue = 0x0;		   // The total value of the int32 data at this offset over all objects in GObjects
		bool bIsValidCmpIdxRange = true;   // Whether this value could be a valid FName::ComparisonIndex
	};


	std::vector<ValueInfo> PossibleOffsets;

	constexpr auto MaxAllowedComparisonIndexValue = 0x4000000; // Somewhat arbitrary limit. Make sure this isn't too low for games on FNamePool with lots of names and 0x14 block-size bits

	constexpr auto MaxAllowedAverageComparisonIndexValue = MaxAllowedComparisonIndexValue / 2; // Also somewhat arbitrary limit, but the average value shouldn't be as high as the max allowed one
	constexpr auto MinAllowedAverageComparisonIndexValue = 0x280; // If the average name is below 0x100 it is either the smallest UE application ever, or not the right offset

	constexpr auto LowComparisonIndexUpperCap = 0x10; // The upper limit of what is considered a "low" comparison index
	constexpr auto MaxAllowedNamesWithLowCmpIdx = 0x40;


	for (int i = sizeof(void*); i <= 0x40; i += 0x4)
	{
		if (!IsPotentialValidOffset(i))
			continue;

		PossibleOffsets.push_back(ValueInfo{ i });
	}

	auto GetDataAtOffsetAsInt = [](const void* Ptr, int32 Offset) -> uint32 { return *reinterpret_cast<const uint32*>(reinterpret_cast<const uintptr_t>(Ptr) + Offset); };

	int NumObjectsConsidered = 0;

	for (; DataSetStartIterator != DataSetEndIterator; ++DataSetStartIterator)
	{
		constexpr auto X86SmallPageSize = 0x1000;
		constexpr auto MaxAccessedSizeInUObject = 0x44;

		const void* CurrentObjectOrField = (*DataSetStartIterator).GetAddress();

		/*
		* Purpose: Make sure all offsets in the UObject::Name finder can be accessed
		* Reasoning: Objects are allocated in Blocks, these allocations are page-aligned in both size and base. If an object + MaxAccessedSizeInUObject goes past the page-bounds
		*            it might also go past the extends of an allocation. There's no reliable way of getting the size of UObject without knowing it's offsets first.
		*/
		const bool bIsGoingPastPageBounds = (reinterpret_cast<const uintptr_t>(CurrentObjectOrField) & (X86SmallPageSize - 1)) > (X86SmallPageSize - MaxAccessedSizeInUObject);
		if (bIsGoingPastPageBounds)
			continue;

		NumObjectsConsidered++;

		for (ValueInfo& Info : PossibleOffsets)
		{
			const uint32 ValueAtOffset = GetDataAtOffsetAsInt(CurrentObjectOrField, Info.Offset);

			Info.TotalValue += ValueAtOffset;
			Info.bIsValidCmpIdxRange = Info.bIsValidCmpIdxRange && ValueAtOffset < MaxAllowedComparisonIndexValue;
			Info.NumNamesWithLowCmpIdx += (ValueAtOffset <= LowComparisonIndexUpperCap);
		}
	}

	int32 FirstValidOffset = -1;
	for (const ValueInfo& Info : PossibleOffsets)
	{
		const auto AverageValue = (Info.TotalValue / NumObjectsConsidered);

		if (Info.bIsValidCmpIdxRange && Info.NumNamesWithLowCmpIdx <= MaxAllowedNamesWithLowCmpIdx
			&& AverageValue >= MinAllowedAverageComparisonIndexValue && AverageValue <= MaxAllowedAverageComparisonIndexValue)
		{
			if (FirstValidOffset == -1)
			{
				FirstValidOffset = Info.Offset;
				continue;
			}

			/* This shouldn't be the case, so log it as an info but continue, as the first offset is still likely the right one. */
			std::cerr << std::format("Dumper-7: Another [UObject/FField]::Name offset (0x{:04X}) is also considered valid.\n", Info.Offset);
		}
	}

	return FirstValidOffset;
}

int32_t OffsetFinder::FindUObjectNameOffset()
{
	auto IsPotentiallyValidOffset = [](int32 Offset) -> bool
	{
		// Make sure 0x4 aligned Offsets are neither the start, nor the middle of a pointer-member. Irrelevant for 32-bit, because the 2nd check will be 0x2 aligned then.
		return Offset != Off::UObject::Class && Offset != (Off::UObject::Class + (sizeof(void*) / 2))
			&& Offset != Off::UObject::Outer && Offset != (Off::UObject::Outer + (sizeof(void*) / 2))
			&& Offset != Off::UObject::Flags
			&& Offset != Off::UObject::Index
			&& Offset != Off::UObject::Vft && Offset != (Off::UObject::Vft + (sizeof(void*) / 2));
	};

	return FindNameOffsetForSomeClass(IsPotentiallyValidOffset, ObjectArray().begin(), ObjectArray().end());
}

int32_t OffsetFinder::FindUObjectOuterOffset()
{
	int32_t LowestFoundOffset = 0xFFFF;

	// loop a few times in case we accidentally choose a UPackage (which doesn't have an Outer) to find Outer
	for (int i = 0; i < 0x10; i++)
	{
		int32_t Offset = 0;

		const void* ObjA = ObjectArray::GetByIndex(rand() % 0x400).GetAddress();
		const void* ObjB = ObjectArray::GetByIndex(rand() % 0x400).GetAddress();

		while (Offset != OffsetNotFound)
		{
			Offset = GetValidPointerOffset(ObjA, ObjB, Offset + sizeof(void*), 0x50);

			// Make sure we didn't re-find the Class offset or Index (if the Index filed is a valid pionter for some ungodly reason). 
			if (Offset != Off::UObject::Class && Offset != Off::UObject::Index)
				break;
		}

		if (Offset != OffsetNotFound && Offset < LowestFoundOffset)
			LowestFoundOffset = Offset;
	}

	return LowestFoundOffset == 0xFFFF ? OffsetNotFound : LowestFoundOffset;
}

void OffsetFinder::FixupHardcodedOffsets()
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

		const int32 OffsetToCheck = Off::FField::Owner + 0x8;
		void* PossibleNextPtrOrBool0 = *(void**)((uint8*)ObjectArray::FindClassFast("Actor").GetChildProperties().GetAddress() + OffsetToCheck);
		void* PossibleNextPtrOrBool1 = *(void**)((uint8*)ObjectArray::FindClassFast("ActorComponent").GetChildProperties().GetAddress() + OffsetToCheck);
		void* PossibleNextPtrOrBool2 = *(void**)((uint8*)ObjectArray::FindClassFast("Pawn").GetChildProperties().GetAddress() + OffsetToCheck);

		auto IsValidPtr = [](void* a) -> bool
		{
			return !IsBadReadPtr(a) && (uintptr_t(a) & 0x1) == 0; // realistically, there wont be any pointers to unaligned memory
		};

		if (IsValidPtr(PossibleNextPtrOrBool0) && IsValidPtr(PossibleNextPtrOrBool1) && IsValidPtr(PossibleNextPtrOrBool2))
		{
			std::cerr << "Applaying fix to hardcoded offsets \n" << std::endl;

			Settings::Internal::bUseMaskForFieldOwner = true;

			Off::FField::Next -= 0x08;
			Off::FField::Name -= 0x08;
			Off::FField::Flags -= 0x08;
		}
	}
}

void OffsetFinder::InitFNameSettings()
{
	UEObject FirstObject = ObjectArray::GetByIndex(0);

	const uint8* NameAddress = static_cast<const uint8*>(FirstObject.GetFName().GetAddress());

	const int32 FNameFirstInt /* ComparisonIndex */ = *reinterpret_cast<const int32*>(NameAddress);
	const int32 FNameSecondInt /* [Number/DisplayIndex] */ = *reinterpret_cast<const int32*>(NameAddress + 0x4);

	/* Some games move 'Name' before 'Class'. Just substract the offset of 'Name' with the offset of the member that follows right after it, to get an estimate of sizeof(FName). */
	const int32 FNameSize = !Settings::Internal::bIsObjectNameBeforeClass ? (Off::UObject::Outer - Off::UObject::Name) : (Off::UObject::Class - Off::UObject::Name);

	Off::FName::CompIdx = 0x0;
	Off::FName::Number = 0x4; // defaults for check

	 // FNames for which FName::Number == [1...4]
	auto GetNumNamesWithNumberOneToFour = []() -> int32
	{
		int32 NamesWithNumberOneToFour = 0x0;

		for (UEObject Obj : ObjectArray())
		{
			const uint32 Number = Obj.GetFName().GetNumber();

			if (Number > 0x0 && Number < 0x5)
				NamesWithNumberOneToFour++;
		}

		return NamesWithNumberOneToFour;
	};

	/*
	* Games without FNAME_OUTLINE_NUMBER have a min. percentage of 6% of all object-names for which FName::Number is in a [1...4] range
	* On games with FNAME_OUTLINE_NUMBER the (random) integer after FName::ComparisonIndex is in the range from [1...4] about 2% (or less) of times.
	*
	* The minimum percentage of names is set to 3% to give both normal names, as well as outline-numer names a buffer-zone.
	*
	* This doesn't work on some very small UE template games, which is why PostInitFNameSettings() was added to fix the incorrect behavior of this function
	*/
	constexpr float MinPercentage = 0.03f;

	/* Minimum required ammount of names for which FName::Number is in a [1...4] range */
	const int32 FNameNumberThreashold = (ObjectArray::Num() * MinPercentage);

	Off::FName::CompIdx = 0x0;

	if (FNameSize == 0x8 && FNameFirstInt == FNameSecondInt) /* WITH_CASE_PRESERVING_NAME + FNAME_OUTLINE_NUMBER */
	{
		Settings::Internal::bUseCasePreservingName = true;
		Settings::Internal::bUseOutlineNumberName = true;

		Off::FName::Number = -0x1;
		Off::InSDK::Name::FNameSize = 0x8;
	}
	else if (FNameSize == 0x10) /* WITH_CASE_PRESERVING_NAME */
	{
		Settings::Internal::bUseCasePreservingName = true;

		Off::FName::Number = FNameFirstInt == FNameSecondInt ? 0x8 : 0x4;

		Off::InSDK::Name::FNameSize = 0xC;
	}
	else if (GetNumNamesWithNumberOneToFour() < FNameNumberThreashold) /* FNAME_OUTLINE_NUMBER */
	{
		Settings::Internal::bUseOutlineNumberName = true;

		Off::FName::Number = -0x1;

		Off::InSDK::Name::FNameSize = 0x4;
	}
	else /* Default */
	{
		Off::FName::Number = 0x4;

		Off::InSDK::Name::FNameSize = 0x8;
	}
}

void OffsetFinder::PostInitFNameSettings()
{
	UEClass PlayerStart = ObjectArray::FindClassFast("PlayerStart");

	const int32 FNameSize = PlayerStart.FindMember("PlayerStartTag").GetSize();

	/* Nothing to do for us, everything is fine! */
	if (Off::InSDK::Name::FNameSize == FNameSize)
		return;

	/* We've used the wrong FNameSize to determine the offset of FField::Flags. Substract the old, wrong, size and add the new one.*/
	Off::FField::Flags = (Off::FField::Flags - Off::InSDK::Name::FNameSize) + FNameSize;

	const uint8* NameAddress = static_cast<const uint8*>(PlayerStart.GetFName().GetAddress());

	const int32 FNameFirstInt /* ComparisonIndex */ = *reinterpret_cast<const int32*>(NameAddress);
	const int32 FNameSecondInt /* [Number/DisplayIndex] */ = *reinterpret_cast<const int32*>(NameAddress + 0x4);

	if (FNameSize == 0x8 && FNameFirstInt == FNameSecondInt) /* WITH_CASE_PRESERVING_NAME + FNAME_OUTLINE_NUMBER */
	{
		Settings::Internal::bUseCasePreservingName = true;
		Settings::Internal::bUseOutlineNumberName = true;

		Off::FName::Number = -0x1;
		Off::InSDK::Name::FNameSize = 0x8;
	}
	else if (FNameSize > 0x8) /* WITH_CASE_PRESERVING_NAME */
	{
		Settings::Internal::bUseOutlineNumberName = false;
		Settings::Internal::bUseCasePreservingName = true;

		Off::FName::Number = FNameFirstInt == FNameSecondInt ? 0x8 : 0x4;

		Off::InSDK::Name::FNameSize = 0xC;
	}
	else if (FNameSize == 0x4) /* FNAME_OUTLINE_NUMBER */
	{
		Settings::Internal::bUseOutlineNumberName = true;
		Settings::Internal::bUseCasePreservingName = false;

		Off::FName::Number = -0x1;

		Off::InSDK::Name::FNameSize = 0x4;
	}
	else /* Default */
	{
		Settings::Internal::bUseOutlineNumberName = false;
		Settings::Internal::bUseCasePreservingName = false;

		Off::FName::Number = 0x4;
		Off::InSDK::Name::FNameSize = 0x8;
	}
}

/* UField */
int32_t OffsetFinder::FindUFieldNextOffset()
{
	const void* KismetSystemLibraryChild = ObjectArray::FindObjectFast<UEStruct>("KismetSystemLibrary").GetChild().GetAddress();
	const void* KismetStringLibraryChild = ObjectArray::FindObjectFast<UEStruct>("KismetStringLibrary").GetChild().GetAddress();

#undef max
	const auto HighestUObjectOffset = std::max({ Off::UObject::Index, Off::UObject::Name, Off::UObject::Flags, Off::UObject::Outer, Off::UObject::Class });
#define max(a,b)            (((a) > (b)) ? (a) : (b))

	return GetValidPointerOffset(KismetSystemLibraryChild, KismetStringLibraryChild, Align(HighestUObjectOffset + 0x4, static_cast<int>(sizeof(void*))), 0x60);
}

/* FField */
int32_t OffsetFinder::FindFFieldNextOffset()
{
	const void* GuidChildren = ObjectArray::FindStructFast("Guid").GetChildProperties().GetAddress();
	const void* VectorChildren = ObjectArray::FindStructFast("Vector").GetChildProperties().GetAddress();

	return GetValidPointerOffset(GuidChildren, VectorChildren, Off::FField::Owner + 0x8, 0x48);
}

int32_t OffsetFinder::FindFFieldNameOffset()
{
	auto IsPotentiallyValidOffset = [](int32 Offset) -> bool
	{
		// Make sure 0x4 aligned Offsets are neither the start, nor the middle of a pointer-member. Irrelevant for 32-bit, because the 2nd check will be 0x2 aligned then.
		return Offset != Off::FField::Class && Offset != (Off::FField::Class + (sizeof(void*) / 2))
			&& Offset != Off::FField::Next && Offset != (Off::FField::Next + (sizeof(void*) / 2))
			&& Offset != Off::FField::Vft && Offset != (Off::FField::Vft + (sizeof(void*) / 2));
	};

	AllFieldIterator TmpIt;

	return FindNameOffsetForSomeClass(IsPotentiallyValidOffset, TmpIt.begin(), TmpIt.end());
}

int32_t OffsetFinder::FindFFieldClassOffset()
{
	const UEFField GuidChild = ObjectArray::FindStructFast("Guid").GetChildProperties();
	const UEFField VectorChild = ObjectArray::FindStructFast("Vector").GetChildProperties();

	return GetValidPointerOffset<false>(GuidChild.GetAddress(), VectorChild.GetAddress(), 0x8, 0x30, true);
}

/* UEnum */
int32_t OffsetFinder::FindEnumNamesOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("ENetRole", EClassCastFlags::Enum).GetAddress(), 0x5 });
	Infos.push_back({ ObjectArray::FindObjectFast("ETraceTypeQuery", EClassCastFlags::Enum).GetAddress(), 0x22 });

	int Ret = FindOffset(Infos) - sizeof(void*);

	if (Ret == (OffsetNotFound - (int32)sizeof(void*)))
	{
		Infos[0] = { ObjectArray::FindObjectFast("EAlphaBlendOption", EClassCastFlags::Enum).GetAddress(), 0x10 };
		Infos[1] = { ObjectArray::FindObjectFast("EUpdateRateShiftBucket", EClassCastFlags::Enum).GetAddress(), 0x8 };

		Ret = FindOffset(Infos) - sizeof(void*);
	}

	using ValueType = std::conditional_t<sizeof(void*) == 0x8, int64, int32>;
	struct Name08Byte { uint8 Pad[0x08]; };
	struct Name16Byte { uint8 Pad[0x10]; };

	uint8* ArrayAddress = static_cast<uint8*>(Infos[0].first) + Ret;

	if (Settings::Internal::bUseCasePreservingName)
	{
		auto& ArrayOfNameValuePairs = *reinterpret_cast<TArray<TPair<Name16Byte, ValueType>>*>(ArrayAddress);

		if (ArrayOfNameValuePairs[1].Second == 1)
			return Ret;

		if constexpr (Settings::EngineCore::bCheckEnumNamesInUEnum)
		{
			if (static_cast<uint8_t>(ArrayOfNameValuePairs[1].Second) == 1 && static_cast<uint8_t>(ArrayOfNameValuePairs[2].Second) == 2)
			{

				Settings::Internal::bIsSmallEnumValue = true;
				return Ret;
			}
		}

		Settings::Internal::bIsEnumNameOnly = true;
	}
	else
	{
		const auto& Array = *reinterpret_cast<TArray<TPair<Name08Byte, ValueType>>*>(static_cast<uint8*>(Infos[0].first) + Ret);

		if (Array[1].Second == 1)
			return Ret;

		if constexpr (Settings::EngineCore::bCheckEnumNamesInUEnum)
		{
			if (static_cast<uint8_t>(Array[1].Second) == 1 && static_cast<uint8_t>(Array[2].Second) == 2)
			{
				Settings::Internal::bIsSmallEnumValue = true;
				return Ret;
			}
		}

		Settings::Internal::bIsEnumNameOnly = true;
	}

	return Ret;
}

/* UStruct */
int32_t OffsetFinder::FindSuperOffset()
{
	std::vector<std::pair<void*, void*>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("Struct").GetAddress(), ObjectArray::FindObjectFast("Field").GetAddress() });
	Infos.push_back({ ObjectArray::FindObjectFast("Class").GetAddress(), ObjectArray::FindObjectFast("Struct").GetAddress() });

	// Thanks to the ue4 dev who decided UStruct should be spelled Ustruct
	if (Infos[0].first == nullptr)
		Infos[0].first = Infos[1].second = ObjectArray::FindObjectFast("struct").GetAddress();

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindChildOffset()
{
	std::vector<std::pair<void*, void*>> Infos;

	if (ObjectArray::FindObject("ObjectProperty Engine.Controller.TransformComponent", EClassCastFlags::ObjectProperty))
	{
		Infos.push_back({ ObjectArray::FindObjectFast("Vector").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector4").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector4").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Vector2D").GetAddress(), ObjectArray::FindObjectFastInOuter("X", "Vector2D").GetAddress() });
		Infos.push_back({ ObjectArray::FindObjectFast("Guid").GetAddress(), ObjectArray::FindObjectFastInOuter("A","Guid").GetAddress() });

		return FindOffset(Infos, 0x14);
	}

	Infos.push_back({ ObjectArray::FindObjectFast("PlayerController").GetAddress(), ObjectArray::FindObjectFastInOuter("WasInputKeyJustReleased", "PlayerController").GetAddress() });
	Infos.push_back({ ObjectArray::FindObjectFast("Controller").GetAddress(), ObjectArray::FindObjectFastInOuter("UnPossess", "Controller").GetAddress() });

	Settings::Internal::bUseFProperty = true;

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindChildPropertiesOffset()
{
	const void* ObjA = ObjectArray::FindStructFast("Color").GetAddress();
	const void* ObjB = ObjectArray::FindStructFast("Guid").GetAddress();

	return GetValidPointerOffset(ObjA, ObjB, Off::UStruct::Children + 0x08, 0x80);
}

int32_t OffsetFinder::FindStructSizeOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("Color").GetAddress(), 0x04 });
	Infos.push_back({ ObjectArray::FindObjectFast("Guid").GetAddress(), 0x10 });

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindMinAlignmentOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("Transform").GetAddress(), 0x10 });

	if constexpr (Settings::Is32Bit())
	{
		Infos.push_back({ ObjectArray::FindObjectFast("InterpCurveLinearColor").GetAddress(), 0x04 });
	}
	else
	{
		Infos.push_back({ ObjectArray::FindObjectFast("PlayerController").GetAddress(), 0x8 });
	}

	return FindOffset(Infos);
}

/* UFunction */
int32_t OffsetFinder::FindFunctionFlagsOffset()
{
	std::vector<std::pair<void*, EFunctionFlags>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("WasInputKeyJustPressed", EClassCastFlags::Function).GetAddress(), EFunctionFlags::Final | EFunctionFlags::Native | EFunctionFlags::Public | EFunctionFlags::BlueprintCallable | EFunctionFlags::BlueprintPure | EFunctionFlags::Const });
	Infos.push_back({ ObjectArray::FindObjectFast("ToggleSpeaking", EClassCastFlags::Function).GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });
	Infos.push_back({ ObjectArray::FindObjectFast("SwitchLevel", EClassCastFlags::Function).GetAddress(), EFunctionFlags::Exec | EFunctionFlags::Native | EFunctionFlags::Public });

	// Some games don't have APlayerController::SwitchLevel(), so we replace it with APlayerController::FOV() which has the same FunctionFlags
	if (Infos[2].first == nullptr)
		Infos[2].first = ObjectArray::FindObjectFast("FOV", EClassCastFlags::Function).GetAddress();

	const int32 Ret = FindOffset(Infos);

	if (Ret != OffsetNotFound)
		return Ret;

	for (auto& [_, Flags] : Infos)
		Flags |= EFunctionFlags::RequiredAPI;

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindFunctionNativeFuncOffset()
{
	std::vector<std::pair<void*, EFunctionFlags>> Infos;

	uintptr_t WasInputKeyJustPressed = reinterpret_cast<uintptr_t>(ObjectArray::FindObjectFast("WasInputKeyJustPressed", EClassCastFlags::Function).GetAddress());
	uintptr_t ToggleSpeaking = reinterpret_cast<uintptr_t>(ObjectArray::FindObjectFast("ToggleSpeaking", EClassCastFlags::Function).GetAddress());
	uintptr_t SwitchLevel_Or_FOV = reinterpret_cast<uintptr_t>(ObjectArray::FindObjectFast("SwitchLevel", EClassCastFlags::Function).GetAddress());

	// Some games don't have APlayerController::SwitchLevel(), so we replace it with APlayerController::FOV() which has the same FunctionFlags
	if (SwitchLevel_Or_FOV == NULL)
		SwitchLevel_Or_FOV = reinterpret_cast<uintptr_t>(ObjectArray::FindObjectFast("FOV", EClassCastFlags::Function).GetAddress());

	for (int i = 0x30; i < 0x140; i += sizeof(void*))
	{
		if (IsInProcessRange(*reinterpret_cast<uintptr_t*>(WasInputKeyJustPressed + i)) && IsInProcessRange(*reinterpret_cast<uintptr_t*>(ToggleSpeaking + i)) && IsInProcessRange(*reinterpret_cast<uintptr_t*>(SwitchLevel_Or_FOV + i)))
			return i;
	}

	return 0x0;
}

/* UClass */
int32_t OffsetFinder::FindCastFlagsOffset()
{
	std::vector<std::pair<void*, EClassCastFlags>> Infos;

	Infos.push_back({ ObjectArray::FindObjectFast("Actor").GetAddress(), EClassCastFlags::Actor });
	Infos.push_back({ ObjectArray::FindObjectFast("Class").GetAddress(), EClassCastFlags::Field | EClassCastFlags::Struct | EClassCastFlags::Class });

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindDefaultObjectOffset()
{
	std::vector<std::pair<void*, void*>> Infos;

	Infos.push_back({ ObjectArray::FindClassFast("Object").GetAddress(), ObjectArray::FindObjectFast("Default__Object").GetAddress() });
	Infos.push_back({ ObjectArray::FindClassFast("Field").GetAddress(), ObjectArray::FindObjectFast("Default__Field").GetAddress() });

	return FindOffset(Infos, 0x28, 0x200);
}

int32_t OffsetFinder::FindImplementedInterfacesOffset()
{
	UEClass Interface_AssetUserDataClass = ObjectArray::FindClassFast("Interface_AssetUserData");

	const uint8_t* ActorComponentClassPtr = reinterpret_cast<const uint8_t*>(ObjectArray::FindClassFast("ActorComponent").GetAddress());

	for (int i = Off::UClass::ClassDefaultObject; i <= (0x350 - 0x10); i += sizeof(void*))
	{
		const auto& ActorArray = *reinterpret_cast<const TArray<FImplementedInterface>*>(ActorComponentClassPtr + i);

		if (ActorArray.IsValid() && !IsBadReadPtr(ActorArray.GetDataPtr()))
		{
			if (ActorArray[0].InterfaceClass == Interface_AssetUserDataClass)
				return i;
		}
	}

	return OffsetNotFound;
}

/* Property */
int32_t OffsetFinder::FindElementSizeOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	UEStruct Guid = ObjectArray::FindStructFast("Guid");

	Infos.push_back({ Guid.FindMember("A").GetAddress(), 0x04 });
	Infos.push_back({ Guid.FindMember("C").GetAddress(), 0x04 });
	Infos.push_back({ Guid.FindMember("D").GetAddress(), 0x04 });

	return FindOffset(Infos);
}

int32_t OffsetFinder::FindArrayDimOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	UEStruct Guid = ObjectArray::FindStructFast("Guid");

	Infos.push_back({ Guid.FindMember("A").GetAddress(), 0x01 });
	Infos.push_back({ Guid.FindMember("C").GetAddress(), 0x01 });
	Infos.push_back({ Guid.FindMember("D").GetAddress(), 0x01 });

	const int32_t MinOffset = Off::Property::ElementSize - 0x10;
	const int32_t MaxOffset = Off::Property::ElementSize + 0x10;

	return FindOffset(Infos, MinOffset, MaxOffset);
}

int32_t OffsetFinder::FindPropertyFlagsOffset()
{
	std::vector<std::pair<void*, EPropertyFlags>> Infos;


	UEStruct Guid = ObjectArray::FindStructFast("Guid");
	UEStruct Color = ObjectArray::FindStructFast("Color");

	constexpr EPropertyFlags GuidMemberFlags = EPropertyFlags::Edit | EPropertyFlags::ZeroConstructor | EPropertyFlags::SaveGame | EPropertyFlags::IsPlainOldData | EPropertyFlags::NoDestructor | EPropertyFlags::HasGetValueTypeHash;
	constexpr EPropertyFlags ColorMemberFlags = EPropertyFlags::Edit | EPropertyFlags::BlueprintVisible | EPropertyFlags::ZeroConstructor | EPropertyFlags::SaveGame | EPropertyFlags::IsPlainOldData | EPropertyFlags::NoDestructor | EPropertyFlags::HasGetValueTypeHash;

	Infos.push_back({ Guid.FindMember("A").GetAddress(), GuidMemberFlags });
	Infos.push_back({ Color.FindMember("R").GetAddress(), ColorMemberFlags });

	if (Infos[1].first == nullptr) [[unlikely]]
		Infos[1].first = Color.FindMember("r").GetAddress();

	int FlagsOffset = FindOffset(Infos);

	// Same flags without AccessSpecifier
	if (FlagsOffset == OffsetNotFound)
	{
		Infos[0].second |= EPropertyFlags::NativeAccessSpecifierPublic;
		Infos[1].second |= EPropertyFlags::NativeAccessSpecifierPublic;

		FlagsOffset = FindOffset(Infos);
	}

	return FlagsOffset;
}

int32_t OffsetFinder::FindOffsetInternalOffset()
{
	std::vector<std::pair<void*, int32_t>> Infos;

	const UEStruct Color = ObjectArray::FindStructFast("Color");
	const UEStruct Guid = ObjectArray::FindStructFast("Guid");

	Infos.push_back({ Color.FindMember("B").GetAddress(), 0x00 });
	Infos.push_back({ Color.FindMember("G").GetAddress(), 0x01 });
	Infos.push_back({ Guid.FindMember("C").GetAddress(), 0x08 });

	// Thanks to the ue5 dev who decided FColor::R should be spelled FColor::r
	if (Infos[2].first == nullptr) [[unlikely]]
		Infos[2].first = Color.FindMember("r").GetAddress();

	return FindOffset(Infos);
}

/* BoolProperty */
int32_t OffsetFinder::FindBoolPropertyBaseOffset()
{
	std::vector<std::pair<void*, uint8_t>> Infos;

	UEClass Engine = ObjectArray::FindClassFast("Engine");
	Infos.push_back({ Engine.FindMember("bIsOverridingSelectedColor").GetAddress(), 0xFF });
	Infos.push_back({ Engine.FindMember("bEnableOnScreenDebugMessagesDisplay").GetAddress(), 0b00000010 });
	Infos.push_back({ ObjectArray::FindClassFast("PlayerController").FindMember("bAutoManageActiveCameraTarget").GetAddress(), 0xFF });

	return (FindOffset<1>(Infos, Off::Property::Offset_Internal) - 0x3);
}

/* ObjectPrperty */
int32_t OffsetFinder::FindObjectPropertyClassOffset()
{
	std::vector<std::pair<void*, void*>> Infos;

	const UEClass Controller = ObjectArray::FindClassFast("Controller");
	Infos.push_back({ Controller.FindMember("PlayerState").GetAddress(), ObjectArray::FindClassFast("PlayerState").GetAddress() });
	Infos.push_back({ Controller.FindMember("Pawn").GetAddress(), ObjectArray::FindClassFast("Pawn").GetAddress() });
	Infos.push_back({ ObjectArray::FindClassFast("World").FindMember("PersistentLevel").GetAddress(), ObjectArray::FindClassFast("Level").GetAddress() });

	return FindOffset(Infos, Off::Property::Offset_Internal);
}

/* EnumProperty */
int32_t OffsetFinder::FindEnumPropertyBaseOffset()
{
	std::vector<std::pair<void*, const void*>> Infos;

	const void* ComponentCreationMethod = ObjectArray::FindObjectFast("EComponentCreationMethod", EClassCastFlags::Enum).GetAddress();
	const void* AutoPossessAI = ObjectArray::FindObjectFast("EAutoPossessAI", EClassCastFlags::Enum).GetAddress();

	if (!ComponentCreationMethod || !AutoPossessAI)
		return OffsetNotFound;

	void* CreationMethodMember = ObjectArray::FindClassFast("ActorComponent").FindMember("CreationMethod", EClassCastFlags::EnumProperty).GetAddress();
	void* AutoPossessAIMember = ObjectArray::FindClassFast("Pawn").FindMember("AutoPossessAI", EClassCastFlags::EnumProperty).GetAddress();

	// UE4.15 and below don't have EnumProperty
	if (!CreationMethodMember || !AutoPossessAIMember)
		return OffsetNotFound;

	Infos.push_back({ CreationMethodMember, ComponentCreationMethod });
	Infos.push_back({ AutoPossessAIMember , AutoPossessAI });

	// EnumProperty::Enum is the 2nd member after 'NumericProperty UnderlayingType'
	return FindOffset(Infos, Off::Property::Offset_Internal) - sizeof(void*);
}

/* ByteProperty */
int32_t OffsetFinder::FindBytePropertyEnumOffset()
{
	std::vector<std::pair<void*, const void*>> Infos;

	const void* CollisionResponseEnum = ObjectArray::FindObjectFast("ECollisionResponse", EClassCastFlags::Enum).GetAddress();

	const UEStruct CollisionResponseContainer = ObjectArray::FindStructFast("CollisionResponseContainer");

	if (!CollisionResponseEnum || !CollisionResponseContainer)
		return OffsetNotFound;

	const void* GameTraceChannel1 = CollisionResponseContainer.FindMember("GameTraceChannel1", EClassCastFlags::ByteProperty).GetAddress();
	const void* GameTraceChannel2 = CollisionResponseContainer.FindMember("GameTraceChannel2", EClassCastFlags::ByteProperty).GetAddress();

	if (!GameTraceChannel1 || !GameTraceChannel2)
		return OffsetNotFound;

	Infos.push_back({ const_cast<void*>(GameTraceChannel1), CollisionResponseEnum });
	Infos.push_back({ const_cast<void*>(GameTraceChannel2), CollisionResponseEnum });

	return FindOffset(Infos, Off::Property::Offset_Internal);
}

/* StructProperty */
int32_t OffsetFinder::FindStructPropertyStructOffset()
{
	std::vector<std::pair<void*, const void*>> Infos;

	const void* VectorClass = ObjectArray::FindStructFast("Vector").GetAddress();

	if (VectorClass == nullptr)
		VectorClass = ObjectArray::FindClassFast("vector").GetAddress();

	const UEStruct TwoVectorsStruct = ObjectArray::FindStructFast("TwoVectors");

	if (!VectorClass || !TwoVectorsStruct)
		return OffsetNotFound;

	const void* v1 = TwoVectorsStruct.FindMember("v1", EClassCastFlags::StructProperty).GetAddress();
	const void* v2 = TwoVectorsStruct.FindMember("v2", EClassCastFlags::StructProperty).GetAddress();

	if (!v1 || !v2)
		return OffsetNotFound;

	Infos.push_back({ const_cast<void*>(v1), VectorClass });
	Infos.push_back({ const_cast<void*>(v2), VectorClass });

	return FindOffset(Infos, Off::Property::Offset_Internal);
}

/* DelegateProperty */
int32_t OffsetFinder::FindDelegatePropertySignatureFunctionOffset()
{
	std::vector<std::pair<void*, const void*>> Infos;

	const void* DelegateSignature = ObjectArray::FindObjectFast("TimerDynamicDelegate__DelegateSignature", EClassCastFlags::Function).GetAddress();

	const UEStruct TwoVectorsStruct = ObjectArray::FindStructFast("TwoVectors");

	if (!DelegateSignature || !TwoVectorsStruct)
		return OffsetNotFound;

	const void* Delegate1 = ObjectArray::FindObjectFast<UEFunction>("K2_GetTimerElapsedTimeDelegate", EClassCastFlags::Function).FindMember("Delegate", EClassCastFlags::DelegateProperty).GetAddress();
	const void* Delegate2 = ObjectArray::FindObjectFast<UEFunction>("K2_GetTimerRemainingTimeDelegate", EClassCastFlags::Function).FindMember("Delegate", EClassCastFlags::DelegateProperty).GetAddress();

	if (!Delegate1 || !Delegate2)
		return OffsetNotFound;

	Infos.push_back({ const_cast<void*>(Delegate1), DelegateSignature });
	Infos.push_back({ const_cast<void*>(Delegate2), DelegateSignature });

	return FindOffset(Infos, Off::Property::Offset_Internal);
}

/* ArrayProperty */
int32_t OffsetFinder::FindInnerTypeOffset(const int32 PropertySize)
{
	if (!Settings::Internal::bUseFProperty)
		return PropertySize;

	if (const UEProperty Property = ObjectArray::FindClassFast("GameViewportClient").FindMember("DebugProperties", EClassCastFlags::ArrayProperty))
	{
		void* AddressToCheck = *reinterpret_cast<void* const*>(reinterpret_cast<const uint8*>(Property.GetAddress()) + PropertySize);

		if (IsBadReadPtr(AddressToCheck))
			return PropertySize + sizeof(void*);
	}

	return PropertySize;
}

/* SetProperty */
int32_t OffsetFinder::FindSetPropertyBaseOffset(const int32 PropertySize)
{
	if (!Settings::Internal::bUseFProperty)
		return PropertySize;

	if (const auto Object = ObjectArray::FindStructFast("LevelCollection").FindMember("Levels", EClassCastFlags::SetProperty))
	{
		const void* AddressToCheck = *reinterpret_cast<void* const*>(reinterpret_cast<const uint8*>(Object.GetAddress()) + PropertySize);

		if (IsBadReadPtr(AddressToCheck))
			return PropertySize + sizeof(void*);
	}

	return PropertySize;
}


/* MapProperty */
int32_t OffsetFinder::FindMapPropertyBaseOffset(const int32 PropertySize)
{
	if (!Settings::Internal::bUseFProperty)
		return PropertySize;

	if (const auto Object = ObjectArray::FindClassFast("UserDefinedEnum").FindMember("DisplayNameMap", EClassCastFlags::MapProperty))
	{
		const void* AddressToCheck = *reinterpret_cast<void* const*>(reinterpret_cast<const uint8*>(Object.GetAddress()) + PropertySize);

		if (IsBadReadPtr(AddressToCheck))
			return PropertySize + sizeof(void*);
	}

	return PropertySize;
}

/* InSDK -> ULevel */
int32_t OffsetFinder::FindLevelActorsOffset()
{
	UEObject Level = nullptr;
	uintptr_t Lvl = 0x0;

	for (auto Obj : ObjectArray())
	{
		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject) || !Obj.IsA(EClassCastFlags::Level))
			continue;

		Level = Obj;
		Lvl = reinterpret_cast<uintptr_t>(Obj.GetAddress());
		break;
	}

	if (Lvl == 0x0)
		return OffsetNotFound;

	/*
	class ULevel : public UObject
	{
		FURL URL;
		TArray<AActor*> Actors;
		TArray<AActor*> GCActors;
	};

	SearchStart = sizeof(UObject) + sizeof(FURL)
	SearchEnd = offsetof(ULevel, OwningWorld)
	*/
	int32 SearchStart = ObjectArray::FindClassFast("Object").GetStructSize() + ObjectArray::FindObjectFast<UEStruct>("URL", EClassCastFlags::Struct).GetStructSize();
	int32 SearchEnd = Level.GetClass().FindMember("OwningWorld").GetOffset();

	for (int i = SearchStart; i <= (SearchEnd - 0x10); i += sizeof(void*))
	{
		const TArray<void*>& ActorArray = *reinterpret_cast<TArray<void*>*>(Lvl + i);

		if (ActorArray.IsValid() && !IsBadReadPtr(ActorArray.GetDataPtr()))
		{
			return i;
		}
	}

	return OffsetNotFound;
}


/* InSDK -> UDataTable */
int32_t OffsetFinder::FindDatatableRowMapOffset()
{
	const UEClass DataTable = ObjectArray::FindClassFast("DataTable");

	constexpr int32 UObjectOuterSize = sizeof(void*);
	constexpr int32 RowStructSize = sizeof(void*);

	if (!DataTable)
	{
		std::cerr << "\nDumper-7: [DataTable] Couldn't find \"DataTable\" class, assuming default layout.\n" << std::endl;
		return (Off::UObject::Outer + UObjectOuterSize + RowStructSize);
	}

	UEProperty RowStructProp = DataTable.FindMember("RowStruct", EClassCastFlags::ObjectProperty);

	if (!RowStructProp)
	{
		std::cerr << "\nDumper-7: [DataTable] Couldn't find \"RowStruct\" property, assuming default layout.\n" << std::endl;
		return (Off::UObject::Outer + UObjectOuterSize + RowStructSize);
	}

	return RowStructProp.GetOffset() + RowStructProp.GetSize();
}

