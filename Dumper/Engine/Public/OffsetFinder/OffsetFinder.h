#pragma once

#include <vector>

#include "Unreal/ObjectArray.h"

namespace OffsetFinder
{
	constexpr int32 OffsetNotFound = -1;

	template<int Alignement = 4, typename T>
	inline int32_t FindOffset(const std::vector<std::pair<void*, T>>& ObjectValuePair, int MinOffset = 0x28, int MaxOffset = 0x1A0)
	{
		int32_t HighestFoundOffset = MinOffset;

		for (int i = 0; i < ObjectValuePair.size(); i++)
		{
			if (ObjectValuePair[i].first == nullptr)
			{
				std::cout << "Dumper-7 ERROR: FindOffset is skipping ObjectValuePair[" << i << "] because .first is nullptr." << std::endl;
				continue;
			}

			for (int j = HighestFoundOffset; j < MaxOffset; j += Alignement)
			{
				const T TypedValueAtOffset = *reinterpret_cast<T*>(static_cast<uint8_t*>(ObjectValuePair[i].first) + j);

				if (TypedValueAtOffset == ObjectValuePair[i].second && j >= HighestFoundOffset)
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

		return HighestFoundOffset != MinOffset ? HighestFoundOffset : OffsetNotFound;
	}

	template<bool bCheckForVft = true>
	inline int32_t GetValidPointerOffset(const uint8_t* ObjA, const uint8_t* ObjB, int32_t StartingOffset, int32_t MaxOffset)
	{
		if (IsBadReadPtr(ObjA) || IsBadReadPtr(ObjB))
			return OffsetNotFound;

		for (int j = StartingOffset; j <= MaxOffset; j += sizeof(void*))
		{
			const bool bIsAValid = !IsBadReadPtr(*reinterpret_cast<void* const*>(ObjA + j)) && (bCheckForVft ? !IsBadReadPtr(**reinterpret_cast<void** const*>(ObjA + j)) : true);
			const bool bIsBValid = !IsBadReadPtr(*reinterpret_cast<void* const*>(ObjB + j)) && (bCheckForVft ? !IsBadReadPtr(**reinterpret_cast<void** const*>(ObjB + j)) : true);

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
