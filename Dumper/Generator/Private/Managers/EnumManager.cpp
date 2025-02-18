#include "Managers/EnumManager.h"

namespace EnumInitHelper
{
	template<typename T>
	constexpr inline uint64 GetMaxOfType()
	{
		return (1ull << (sizeof(T) * 0x8ull)) - 1;
	}

	void SetEnumSizeForValue(uint8& Size, uint64 EnumValue)
	{
		if (EnumValue > GetMaxOfType<uint32>()) {
			Size = 0x8;
		}
		else if (EnumValue > GetMaxOfType<uint16>()) {
			Size = max(Size, 0x4);
		}
		else if (EnumValue > GetMaxOfType<uint8>()) {
			Size = max(Size, 0x2);
		}
		else {
			Size = max(Size, 0x1);
		}
	}
}

std::string EnumCollisionInfo::GetUniqueName() const
{
	const std::string Name = EnumManager::GetValueName(*this).GetName();

	if (CollisionCount > 0)
		return Name + "_" + std::to_string(CollisionCount - 1);

	return Name;
}

std::string EnumCollisionInfo::GetRawName() const
{
	return EnumManager::GetValueName(*this).GetName();
}

uint64 EnumCollisionInfo::GetValue() const
{
	return MemberValue;
}

uint8 EnumCollisionInfo::GetCollisionCount() const
{
	return CollisionCount;
}

EnumInfoHandle::EnumInfoHandle(const EnumInfo& InInfo)
	: Info(&InInfo)
{
}

uint8 EnumInfoHandle::GetUnderlyingTypeSize() const
{
	return Info->UnderlyingTypeSize;
}

const StringEntry& EnumInfoHandle::GetName() const
{
	return EnumManager::GetEnumName(*Info);
}

int32 EnumInfoHandle::GetNumMembers() const
{
	return Info->MemberInfos.size();
}

CollisionInfoIterator EnumInfoHandle::GetMemberCollisionInfoIterator() const
{
	return CollisionInfoIterator(Info->MemberInfos);
}


void EnumManager::InitInternal()
{
	for (auto Obj : ObjectArray())
	{
		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject))
			continue;

		if (Obj.IsA(EClassCastFlags::Struct))
		{
			UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

			for (UEProperty Property : ObjAsStruct.GetProperties())
			{
				if (!Property.IsA(EClassCastFlags::EnumProperty) && !Property.IsA(EClassCastFlags::ByteProperty))
					continue;

				UEEnum Enum = nullptr;
				UEProperty UnderlayingProperty = nullptr;

				if (Property.IsA(EClassCastFlags::EnumProperty))
				{
					Enum = Property.Cast<UEEnumProperty>().GetEnum();
					UnderlayingProperty = Property.Cast<UEEnumProperty>().GetUnderlayingProperty();

					if (!UnderlayingProperty)
						continue;
				}
				else /* ByteProperty */
				{
					Enum = Property.Cast<UEByteProperty>().GetEnum();
					UnderlayingProperty = Property;
				}

				if (!Enum)
					continue;

				EnumInfo& Info = EnumInfoOverrides[Enum.GetIndex()];

				Info.bWasInstanceFound = true;
				Info.UnderlyingTypeSize = 0x1;

				/* Check if the size of this enums underlaying type is greater than the default size (0x1) */
				if (Enum)
				{
					Info.UnderlyingTypeSize = Property.GetSize();
					continue;
				}

				if (UnderlayingProperty)
				{
					Info.UnderlyingTypeSize = UnderlayingProperty.GetSize();
					continue;
				}
			}
		}
		else if (Obj.IsA(EClassCastFlags::Enum))
		{
			UEEnum ObjAsEnum = Obj.Cast<UEEnum>();

			/* Add name to override info */
			EnumInfo& NewOrExistingInfo = EnumInfoOverrides[Obj.GetIndex()];
			NewOrExistingInfo.Name = UniqueEnumNameTable.FindOrAdd(ObjAsEnum.GetEnumPrefixedName()).first;

			uint64 EnumMaxValue = 0x0;

			/* Initialize enum-member names and their collision infos */
			std::vector<std::pair<FName, int64>> NameValuePairs = ObjAsEnum.GetNameValuePairs();
			for (int i = 0; i < NameValuePairs.size(); i++)
			{
				auto& [Name, Value] = NameValuePairs[i];

				std::wstring NameWitPrefix = Name.ToWString();

				if (!NameWitPrefix.ends_with(L"_MAX"))
					EnumMaxValue = max(EnumMaxValue, Value);

				auto [NameIndex, bWasInserted] = UniqueEnumValueNames.FindOrAdd(MakeNameValid(NameWitPrefix.substr(NameWitPrefix.find_last_of(L"::") + 1)));

				EnumCollisionInfo CurrentEnumValueInfo;
				CurrentEnumValueInfo.MemberName = NameIndex;
				CurrentEnumValueInfo.MemberValue = Value;

				if (bWasInserted) [[likely]]
				{
					NewOrExistingInfo.MemberInfos.push_back(CurrentEnumValueInfo);
					continue;
				}

				/* A value with this name exists globally, now check if it also exists localy (aka. is duplicated) */
				for (int j = 0; j < i; j++)
				{
					EnumCollisionInfo& CrosscheckedInfo = NewOrExistingInfo.MemberInfos[j];

					if (CrosscheckedInfo.MemberName != NameIndex) [[likely]]
						continue;

					/* Duplicate was found */
					CurrentEnumValueInfo.CollisionCount = CrosscheckedInfo.CollisionCount + 1;
					break;
				}

				/* Check if this name is illegal */
				for (HashStringTableIndex IllegalIndex : IllegalNames)
				{
					if (NameIndex == IllegalIndex) [[unlikely]]
					{
						CurrentEnumValueInfo.CollisionCount++;
						break;
					}
				}

				NewOrExistingInfo.MemberInfos.push_back(CurrentEnumValueInfo);
			}

			/* Initialize the size based on the highest value contained by this enum */
			if (!NewOrExistingInfo.bWasEnumSizeInitialized && !NewOrExistingInfo.bWasInstanceFound)
			{
				EnumInitHelper::SetEnumSizeForValue(NewOrExistingInfo.UnderlyingTypeSize, EnumMaxValue);
				NewOrExistingInfo.bWasEnumSizeInitialized = true;
			}
		}
	}
}

void EnumManager::InitIllegalNames()
{
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("IN").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("OUT").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("TRUE").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("FALSE").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("DELETE").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("PF_MAX").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("SW_MAX").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("MM_MAX").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("SIZE_MAX").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("RELATIVE").first);
	IllegalNames.push_back(UniqueEnumValueNames.FindOrAdd("TRANSPARENT").first);
}

void EnumManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	EnumInfoOverrides.reserve(0x1000);

	InitIllegalNames(); // call this first
	InitInternal();
}