#pragma once

#include <vector>

#include "Unreal/ObjectArray.h"

namespace OffsetFinder
{
	constexpr int32 OffsetNotFound = -1;
	constexpr int32 OffsetFinderMinValue = Settings::Is32Bit() ? 0x18 : 0x28;

	template<int Alignement = 4, typename T>
	inline int32_t FindOffset(const std::vector<std::pair<void*, T>>& ObjectValuePair, int MinOffset = OffsetFinderMinValue, int MaxOffset = 0x1A0)
	{
		int32_t HighestFoundOffset = MinOffset;
		bool bFoundOffset = false;

		for (int i = 0; i < ObjectValuePair.size(); i++)
		{
			if (ObjectValuePair[i].first == nullptr)
			{
				std::cerr << "Dumper-7 ERROR: FindOffset is skipping ObjectValuePair[" << i << "] because .first is nullptr." << std::endl;
				continue;
			}

			for (int j = HighestFoundOffset; j < MaxOffset; j += Alignement)
			{
				const T TypedValueAtOffset = *reinterpret_cast<T*>(static_cast<uint8_t*>(ObjectValuePair[i].first) + j);

				if (TypedValueAtOffset == ObjectValuePair[i].second && j >= HighestFoundOffset)
				{
					bFoundOffset = true;

					if (j > HighestFoundOffset)
					{
						HighestFoundOffset = j;
						i = 0;
					}
					j = MaxOffset;
				}
			}
		}

		//return HighestFoundOffset != MinOffset ? HighestFoundOffset : OffsetNotFound;
		return bFoundOffset ? HighestFoundOffset : OffsetNotFound;
	}

	template<bool bCheckForVft = true>
	inline int32_t GetValidPointerOffset(const void* PtrObjA, const void* PtrObjB, int32_t StartingOffset, int32_t MaxOffset, bool bNeedsToBeInProcessMemory = false)
	{
		const uint8_t* ObjA = static_cast<const uint8_t*>(PtrObjA);
		const uint8_t* ObjB = static_cast<const uint8_t*>(PtrObjB);

		if (IsBadReadPtr(ObjA) || IsBadReadPtr(ObjB))
			return OffsetNotFound;

		for (int j = StartingOffset; j <= MaxOffset; j += sizeof(void*))
		{
			const bool bIsAValid = !IsBadReadPtr(*reinterpret_cast<void* const*>(ObjA + j)) && (bCheckForVft ? !IsBadReadPtr(**reinterpret_cast<void** const*>(ObjA + j)) : true);
			const bool bIsBValid = !IsBadReadPtr(*reinterpret_cast<void* const*>(ObjB + j)) && (bCheckForVft ? !IsBadReadPtr(**reinterpret_cast<void** const*>(ObjB + j)) : true);

			if (bNeedsToBeInProcessMemory)
			{
				if (!IsInProcessRange(*reinterpret_cast<void* const*>(ObjA + j)) || !IsInProcessRange(*reinterpret_cast<void* const*>(ObjB + j)))
					continue;
			}

			if (bIsAValid && bIsBValid)
				return j;
		}

		return OffsetNotFound;
	};

	/* UObject */
	int32_t FindUObjectFlagsOffset();
	int32_t FindUObjectIndexOffset();
	int32_t FindUObjectClassOffset();
	int32_t FindUObjectNameOffset();
	int32_t FindUObjectOuterOffset();

	void FixupHardcodedOffsets();
	void InitFNameSettings();
	void PostInitFNameSettings();

	/* UField */
	int32_t FindUFieldNextOffset();

	/* FField */
	int32_t FindFFieldNextOffset();
	int32_t FindFFieldNameOffset();
	int32_t NewFindFFieldNameOffset();
	int32_t FindFFieldClassOffset();

	/* UEnum */
	int32_t FindEnumNamesOffset();

	/* UStruct */
	int32_t FindSuperOffset();
	int32_t FindChildOffset();
	int32_t FindChildPropertiesOffset();
	int32_t FindStructSizeOffset();
	int32_t FindMinAlignmentOffset();

	/* UFunction */
	int32_t FindFunctionFlagsOffset();
	int32_t FindFunctionNativeFuncOffset();

	/* UClass */
	int32_t FindCastFlagsOffset();
	int32_t FindDefaultObjectOffset();
	int32_t FindImplementedInterfacesOffset();

	/* Property */
	int32_t FindElementSizeOffset();
	int32_t FindArrayDimOffset();
	int32_t FindPropertyFlagsOffset();
	int32_t FindOffsetInternalOffset();

	/* BoolProperty */
	int32_t FindBoolPropertyBaseOffset();

	/* ObjectProperty */
	int32_t FindObjectPropertyClassOffset();

	/* EnumProperty */
	int32_t FindEnumPropertyBaseOffset();
	
	/* ByteProperty */
	int32_t FindBytePropertyEnumOffset();

	/* StructProperty */
	int32_t FindStructPropertyStructOffset();

	/* DelegateProperty */
	int32_t FindDelegatePropertySignatureFunctionOffset();

	/* ArrayProperty */
	int32_t FindInnerTypeOffset(const int32 PropertySize);

	/* SetProperty */
	int32_t FindSetPropertyBaseOffset(const int32 PropertySize);

	/* MapProperty */
	int32_t FindMapPropertyBaseOffset(const int32 PropertySize);

	/* InSDK -> ULevel */
	int32_t FindLevelActorsOffset();

	/* InSDK -> UDataTable */
	int32_t FindDatatableRowMapOffset();
}
